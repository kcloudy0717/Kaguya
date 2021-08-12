#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

const char* DxcException::GetErrorType() const noexcept
{
	return "[Dxc]";
}

std::string DxcException::GetError() const
{
#define DXCERR(x)                                                                                                      \
	case x:                                                                                                            \
		Error = #x;                                                                                                    \
		break

	std::string Error;
	switch (ErrorCode)
	{
		DXCERR(E_FAIL);
		DXCERR(E_INVALIDARG);
		DXCERR(E_OUTOFMEMORY);
		DXCERR(E_NOTIMPL);
		DXCERR(E_NOINTERFACE);
	default:
	{
		char Buffer[64] = {};
		sprintf_s(Buffer, "HRESULT of 0x%08X", static_cast<UINT>(ErrorCode));
		Error = Buffer;
	}
	break;
	}
#undef DXCERR
	return Error;
}

void ShaderCompiler::Initialize()
{
	VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler3.ReleaseAndGetAddressOf())));
	VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(Utils.ReleaseAndGetAddressOf())));
	VERIFY_DXC_API(Utils->CreateDefaultIncludeHandler(DefaultIncludeHandler.ReleaseAndGetAddressOf()));
}

void ShaderCompiler::SetShaderModel(EShaderModel ShaderModel) noexcept
{
	this->ShaderModel = ShaderModel;
}

void ShaderCompiler::SetIncludeDirectory(const std::filesystem::path& Path)
{
	IncludeDirectory = Path.wstring();
}

Shader ShaderCompiler::CompileShader(
	EShaderType					  ShaderType,
	const std::filesystem::path&  Path,
	std::wstring_view			  EntryPoint,
	const std::vector<DxcDefine>& ShaderDefines) const
{
	std::wstring ProfileString = ShaderProfileString(ShaderType);

	IDxcBlob*	 Blob	 = nullptr;
	IDxcBlob*	 PDBBlob = nullptr;
	std::wstring PDBName;
	Compile(Path, EntryPoint, ProfileString.data(), ShaderDefines, &Blob, &PDBBlob, PDBName);

	return Shader(ShaderType, Blob, PDBBlob, std::move(PDBName));
}

Library ShaderCompiler::CompileLibrary(const std::filesystem::path& Path) const
{
	std::wstring ProfileString = LibraryProfileString();

	IDxcBlob*	 Blob	 = nullptr;
	IDxcBlob*	 PDBBlob = nullptr;
	std::wstring PDBName;
	Compile(Path, L"", ProfileString.data(), {}, &Blob, &PDBBlob, PDBName);

	return Library(Blob, PDBBlob, std::move(PDBName));
}

Shader ShaderCompiler::SpirVCodeGen(
	EShaderType					  ShaderType,
	const std::filesystem::path&  Path,
	std::wstring_view			  EntryPoint,
	const std::vector<DxcDefine>& ShaderDefines)
{
	std::wstring ProfileString = ShaderProfileString(ShaderType);

	IDxcBlob* Blob = nullptr;
	SpirV(Path, EntryPoint, ProfileString.data(), ShaderDefines, &Blob);

	return Shader(ShaderType, Blob, nullptr, std::wstring());
}

std::wstring ShaderCompiler::GetShaderModelString() const
{
	std::wstring ShaderModelString;
	switch (ShaderModel)
	{
	case EShaderModel::ShaderModel_6_5:
		ShaderModelString = L"6_5";
		break;
	case EShaderModel::ShaderModel_6_6:
		ShaderModelString = L"6_6";
		break;
	}

	return ShaderModelString;
}

std::wstring ShaderCompiler::ShaderProfileString(EShaderType ShaderType) const
{
	std::wstring ProfileString;
	switch (ShaderType)
	{
	case EShaderType::Vertex:
		ProfileString = L"vs_";
		break;
	case EShaderType::Hull:
		ProfileString = L"hs_";
		break;
	case EShaderType::Domain:
		ProfileString = L"ds_";
		break;
	case EShaderType::Geometry:
		ProfileString = L"gs_";
		break;
	case EShaderType::Pixel:
		ProfileString = L"ps_";
		break;
	case EShaderType::Compute:
		ProfileString = L"cs_";
		break;
	case EShaderType::Amplification:
		ProfileString = L"as_";
		break;
	case EShaderType::Mesh:
		ProfileString = L"ms_";
		break;
	}

	return ProfileString + GetShaderModelString();
}

std::wstring ShaderCompiler::LibraryProfileString() const
{
	return L"lib_" + GetShaderModelString();
}

void ShaderCompiler::Compile(
	const std::filesystem::path&  Path,
	std::wstring_view			  EntryPoint,
	std::wstring_view			  Profile,
	const std::vector<DxcDefine>& ShaderDefines,
	_Outptr_result_maybenull_ IDxcBlob** ppBlob,
	_Outptr_result_maybenull_ IDxcBlob** ppPDBBlob,
	std::wstring&						 PDBName) const
{
	*ppBlob	   = nullptr;
	*ppPDBBlob = nullptr;

	// https://developer.nvidia.com/dx12-dos-and-donts
	LPCWSTR Arguments[] = { // Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
							// This allows for the compiler to do a better job at optimizing texture accesses. We have
							// seen frame rate improvements of > 1 % when toggling this flag on.
							L"-all_resources_bound",
#ifdef _DEBUG
							L"-WX",			  // Warnings as errors
							L"-Zi",			  // Debug info
							L"-Qembed_debug", // Embed debug info into the shader
							L"-Od",			  // Disable optimization
#else
							L"-O3", // Optimization level 3
#endif
									// Add include directory
							L"-I",
							IncludeDirectory.data()
	};

	// Build arguments
	ComPtr<IDxcCompilerArgs> DxcCompilerArgs;
	VERIFY_DXC_API(Utils->BuildArguments(
		Path.c_str(),
		EntryPoint.data(),
		Profile.data(),
		Arguments,
		ARRAYSIZE(Arguments),
		ShaderDefines.data(),
		static_cast<UINT32>(ShaderDefines.size()),
		DxcCompilerArgs.ReleaseAndGetAddressOf()));

	ComPtr<IDxcBlobEncoding> Source;
	UINT32					 CodePage = CP_ACP;

	VERIFY_DXC_API(Utils->LoadFile(Path.c_str(), &CodePage, Source.ReleaseAndGetAddressOf()));

	BOOL SourceKnown	= FALSE;
	UINT SourceCodePage = 0;
	VERIFY_DXC_API(Source->GetEncoding(&SourceKnown, &CodePage));

	DxcBuffer DxcBuffer = {};
	DxcBuffer.Ptr		= Source->GetBufferPointer();
	DxcBuffer.Size		= Source->GetBufferSize();
	DxcBuffer.Encoding	= SourceCodePage;

	ComPtr<IDxcResult> DxcResult;
	VERIFY_DXC_API(Compiler3->Compile(
		&DxcBuffer,
		DxcCompilerArgs->GetArguments(),
		DxcCompilerArgs->GetCount(),
		DefaultIncludeHandler.Get(),
		IID_PPV_ARGS(DxcResult.ReleaseAndGetAddressOf())));

	HRESULT Status;
	if (SUCCEEDED(DxcResult->GetStatus(&Status)))
	{
		if (FAILED(Status))
		{
			ComPtr<IDxcBlobUtf8>  Errors;
			ComPtr<IDxcBlobUtf16> OutputName;
			if (SUCCEEDED(DxcResult->GetOutput(
					DXC_OUT_ERRORS,
					IID_PPV_ARGS(Errors.GetAddressOf()),
					OutputName.ReleaseAndGetAddressOf())))
			{
				OutputDebugStringA(std::bit_cast<char*>(Errors->GetBufferPointer()));
				throw std::runtime_error("Failed to compile shader");
			}
			else
			{
				throw std::runtime_error("Failed to obtain error");
			}
		}
	}

	DxcResult->GetResult(ppBlob);
	if (DxcResult->HasOutput(DXC_OUT_PDB))
	{
		ComPtr<IDxcBlobUtf16> PDB;
		DxcResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(ppPDBBlob), PDB.ReleaseAndGetAddressOf());
		PDBName = PDB->GetStringPointer();
	}
}

void ShaderCompiler::SpirV(
	const std::filesystem::path&  Path,
	std::wstring_view			  EntryPoint,
	std::wstring_view			  Profile,
	const std::vector<DxcDefine>& ShaderDefines,
	_Outptr_result_maybenull_ IDxcBlob** ppBlob) const
{
	*ppBlob = nullptr;

	LPCWSTR Arguments[] = { L"-spirv",
#ifdef _DEBUG
							L"-WX", // Warnings as errors
							L"-Zi", // Debug info
							L"-Od", // Disable optimization
#else
							L"-O3", // Optimization level 3
#endif
									// Add include directory
							L"-I",
							IncludeDirectory.data() };

	// Build arguments
	ComPtr<IDxcCompilerArgs> DxcCompilerArgs;
	VERIFY_DXC_API(Utils->BuildArguments(
		Path.c_str(),
		EntryPoint.data(),
		Profile.data(),
		Arguments,
		ARRAYSIZE(Arguments),
		ShaderDefines.data(),
		static_cast<UINT32>(ShaderDefines.size()),
		DxcCompilerArgs.ReleaseAndGetAddressOf()));

	ComPtr<IDxcBlobEncoding> Source;
	UINT32					 CodePage = CP_ACP;

	VERIFY_DXC_API(Utils->LoadFile(Path.c_str(), &CodePage, Source.ReleaseAndGetAddressOf()));

	BOOL SourceKnown	= FALSE;
	UINT SourceCodePage = 0;
	VERIFY_DXC_API(Source->GetEncoding(&SourceKnown, &CodePage));

	DxcBuffer DxcBuffer = {};
	DxcBuffer.Ptr		= Source->GetBufferPointer();
	DxcBuffer.Size		= Source->GetBufferSize();
	DxcBuffer.Encoding	= SourceCodePage;

	ComPtr<IDxcResult> DxcResult;
	VERIFY_DXC_API(Compiler3->Compile(
		&DxcBuffer,
		DxcCompilerArgs->GetArguments(),
		DxcCompilerArgs->GetCount(),
		DefaultIncludeHandler.Get(),
		IID_PPV_ARGS(DxcResult.ReleaseAndGetAddressOf())));

	HRESULT Status;
	if (SUCCEEDED(DxcResult->GetStatus(&Status)))
	{
		if (FAILED(Status))
		{
			ComPtr<IDxcBlobUtf8>  Errors;
			ComPtr<IDxcBlobUtf16> OutputName;
			if (SUCCEEDED(DxcResult->GetOutput(
					DXC_OUT_ERRORS,
					IID_PPV_ARGS(Errors.GetAddressOf()),
					OutputName.ReleaseAndGetAddressOf())))
			{
				OutputDebugStringA(std::bit_cast<char*>(Errors->GetBufferPointer()));
				throw std::runtime_error("Failed to compile shader");
			}
			else
			{
				throw std::runtime_error("Failed to obtain error");
			}
		}
	}

	DxcResult->GetResult(ppBlob);
}
