#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

ShaderCompiler::ShaderCompiler()
{
	ASSERTD3D12APISUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler3.ReleaseAndGetAddressOf())));
	ASSERTD3D12APISUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(Utils.ReleaseAndGetAddressOf())));
	ASSERTD3D12APISUCCEEDED(Utils->CreateDefaultIncludeHandler(DefaultIncludeHandler.ReleaseAndGetAddressOf()));
	ShaderModel = D3D_SHADER_MODEL_6_5;
}

void ShaderCompiler::SetShaderModel(D3D_SHADER_MODEL ShaderModel)
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

std::wstring ShaderCompiler::GetShaderModelString() const
{
	std::wstring ShaderModelString;
	switch (ShaderModel)
	{
	case D3D_SHADER_MODEL_6_3:
		ShaderModelString = L"6_3";
		break;
	case D3D_SHADER_MODEL_6_4:
		ShaderModelString = L"6_4";
		break;
	case D3D_SHADER_MODEL_6_5:
		ShaderModelString = L"6_5";
		break;
	case D3D_SHADER_MODEL_6_6:
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
	ASSERTD3D12APISUCCEEDED(Utils->BuildArguments(
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

	ASSERTD3D12APISUCCEEDED(Utils->LoadFile(Path.c_str(), &CodePage, Source.ReleaseAndGetAddressOf()));

	BOOL SourceKnown	= FALSE;
	UINT SourceCodePage = 0;
	ASSERTD3D12APISUCCEEDED(Source->GetEncoding(&SourceKnown, &CodePage));

	DxcBuffer DxcBuffer = {};
	DxcBuffer.Ptr		= Source->GetBufferPointer();
	DxcBuffer.Size		= Source->GetBufferSize();
	DxcBuffer.Encoding	= SourceCodePage;

	ComPtr<IDxcResult> DxcResult;
	ASSERTD3D12APISUCCEEDED(Compiler3->Compile(
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
