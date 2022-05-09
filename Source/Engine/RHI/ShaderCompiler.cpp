#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

DEFINE_LOG_CATEGORY(Dxc);

#define VERIFY_DXC_API(expr)                            \
	do                                                  \
	{                                                   \
		HRESULT hr = expr;                              \
		if (FAILED(hr))                                 \
		{                                               \
			throw DxcException(__FILE__, __LINE__, hr); \
		}                                               \
	} while (false)

const char* DxcException::GetErrorType() const noexcept
{
	return "[Dxc]";
}

std::string DxcException::GetError() const
{
#define DXCERR(x)   \
	case x:         \
		Error = #x; \
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

ShaderCompiler::ShaderCompiler()
	: ShaderModel(RHI_SHADER_MODEL::ShaderModel_6_5)
{
	VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler3)));
	VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Utils)));
	VERIFY_DXC_API(Utils->CreateDefaultIncludeHandler(&DefaultIncludeHandler));
}

void ShaderCompiler::SetShaderModel(
	RHI_SHADER_MODEL ShaderModel) noexcept
{
	this->ShaderModel = ShaderModel;
}

Shader ShaderCompiler::CompileShader(
	RHI_SHADER_TYPE				 ShaderType,
	const std::filesystem::path& Path,
	const ShaderCompileOptions&	 Options) const
{
	std::wstring ProfileString = ShaderProfileString(ShaderType);

	std::vector<DxcDefine> Defines;
	Defines.reserve(Options.Defines.size());
	for (const auto& Define : Options.Defines)
	{
		Defines.emplace_back(Define.first.data(), Define.second.data());
	}

	ShaderCompilationResult Result = Compile(Path, Options.EntryPoint, ProfileString.data(), Defines);
	return { ShaderType, Result.ShaderHash, Result.Binary, Result.Pdb };
}

Shader ShaderCompiler::CompileVS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Vertex, Path, Options);
}

Shader ShaderCompiler::CompileHS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Hull, Path, Options);
}

Shader ShaderCompiler::CompileDS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Domain, Path, Options);
}

Shader ShaderCompiler::CompileGS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Geometry, Path, Options);
}

Shader ShaderCompiler::CompilePS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Pixel, Path, Options);
}

Shader ShaderCompiler::CompileCS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Compute, Path, Options);
}

Shader ShaderCompiler::CompileAS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Amplification, Path, Options);
}

Shader ShaderCompiler::CompileMS(const std::filesystem::path& Path, const ShaderCompileOptions& Options) const
{
	return CompileShader(RHI_SHADER_TYPE::Mesh, Path, Options);
}

Library ShaderCompiler::CompileLibrary(
	const std::filesystem::path& Path) const
{
	std::wstring ProfileString = LibraryProfileString();

	ShaderCompilationResult Result = Compile(Path, L"", ProfileString.data(), {});
	return { Result.ShaderHash, Result.Binary, Result.Pdb };
}

std::wstring ShaderCompiler::GetShaderModelString() const
{
	std::wstring ShaderModelString;
	switch (ShaderModel)
	{
	case RHI_SHADER_MODEL::ShaderModel_6_5:
		ShaderModelString = L"6_5";
		break;
	case RHI_SHADER_MODEL::ShaderModel_6_6:
		ShaderModelString = L"6_6";
		break;
	}

	return ShaderModelString;
}

std::wstring ShaderCompiler::ShaderProfileString(RHI_SHADER_TYPE ShaderType) const
{
	std::wstring ProfileString;
	switch (ShaderType)
	{
		using enum RHI_SHADER_TYPE;
	case Vertex:
		ProfileString = L"vs_";
		break;
	case Hull:
		ProfileString = L"hs_";
		break;
	case Domain:
		ProfileString = L"ds_";
		break;
	case Geometry:
		ProfileString = L"gs_";
		break;
	case Pixel:
		ProfileString = L"ps_";
		break;
	case Compute:
		ProfileString = L"cs_";
		break;
	case Amplification:
		ProfileString = L"as_";
		break;
	case Mesh:
		ProfileString = L"ms_";
		break;
	}

	return ProfileString + GetShaderModelString();
}

std::wstring ShaderCompiler::LibraryProfileString() const
{
	return L"lib_" + GetShaderModelString();
}

ShaderCompilationResult ShaderCompiler::Compile(
	const std::filesystem::path&  Path,
	std::wstring_view			  EntryPoint,
	std::wstring_view			  Profile,
	const std::vector<DxcDefine>& ShaderDefines) const
{
	ScopedTimer Timer(
		[&](i64 Milliseconds)
		{
			KAGUYA_LOG(Dxc, Info, L"{} compiled in {}ms", Path.c_str(), Milliseconds);
		});

	ShaderCompilationResult Result = {};

	std::filesystem::path PdbPath = Path;
	PdbPath.replace_extension(L"pdb");

	// https://developer.nvidia.com/dx12-dos-and-donts
	LPCWSTR Arguments[] = {
		// Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
		// This allows for the compiler to do a better job at optimizing texture accesses. We have
		// seen frame rate improvements of > 1 % when toggling this flag on.
		L"-all_resources_bound",
		L"-WX", // Warnings as errors
		L"-Zi", // Debug info
		L"-Fd",
		PdbPath.c_str(), // Shader Pdb
#ifdef _DEBUG
		L"-Od", // Disable optimization
#else
		L"-O3", // Optimization level 3
#endif
		L"-Zss", // Compute shader hash based on source
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
		&DxcCompilerArgs));

	ComPtr<IDxcBlobEncoding> Source;
	VERIFY_DXC_API(Utils->LoadFile(Path.c_str(), nullptr, &Source));

	DxcBuffer SourceBuffer = {
		.Ptr	  = Source->GetBufferPointer(),
		.Size	  = Source->GetBufferSize(),
		.Encoding = DXC_CP_ACP
	};
	ComPtr<IDxcResult> DxcResult;
	VERIFY_DXC_API(Compiler3->Compile(
		&SourceBuffer,
		DxcCompilerArgs->GetArguments(),
		DxcCompilerArgs->GetCount(),
		DefaultIncludeHandler.Get(),
		IID_PPV_ARGS(&DxcResult)));

	HRESULT Status = S_FALSE;
	if (SUCCEEDED(DxcResult->GetStatus(&Status)))
	{
		if (FAILED(Status))
		{
			ComPtr<IDxcBlobUtf8>  Errors;
			ComPtr<IDxcBlobUtf16> OutputName;
			VERIFY_DXC_API(DxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), &OutputName));
			KAGUYA_LOG(RHI, Error, "Shader compile error: {}", std::bit_cast<char*>(Errors->GetBufferPointer()));
			throw std::runtime_error("Failed to compile shader");
		}
	}

	DxcResult->GetResult(&Result.Binary);

	if (DxcResult->HasOutput(DXC_OUT_PDB))
	{
		ComPtr<IDxcBlobUtf16> Name;
		DxcResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&Result.Pdb), &Name);
		if (Name)
		{
			Result.PdbName = Name->GetStringPointer();
		}

		// Save pdb
		FileStream	 Stream(PdbPath, FileMode::Create, FileAccess::Write);
		BinaryWriter Writer(Stream);
		Writer.Write(Result.Pdb->GetBufferPointer(), Result.Pdb->GetBufferSize());
	}

	if (DxcResult->HasOutput(DXC_OUT_SHADER_HASH))
	{
		ComPtr<IDxcBlob>	  ShaderHash;
		ComPtr<IDxcBlobUtf16> OutputName;
		DxcResult->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&ShaderHash), &OutputName);
		if (ShaderHash)
		{
			assert(ShaderHash->GetBufferSize() == sizeof(DxcShaderHash));
			Result.ShaderHash = *static_cast<DxcShaderHash*>(ShaderHash->GetBufferPointer());
		}
	}

	return Result;
}
