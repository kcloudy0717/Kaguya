#include "pch.h"
#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

ShaderCompiler::ShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_DxcCompiler.ReleaseAndGetAddressOf())));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_DxcUtils.ReleaseAndGetAddressOf())));
	ThrowIfFailed(m_DxcUtils->CreateDefaultIncludeHandler(m_DxcIncludeHandler.ReleaseAndGetAddressOf()));
	m_ShaderModel = D3D_SHADER_MODEL_6_6;
}

void ShaderCompiler::SetShaderModel(
	D3D_SHADER_MODEL ShaderModel)
{
	m_ShaderModel = ShaderModel;
}

void ShaderCompiler::SetIncludeDirectory(
	const std::filesystem::path& pPath)
{
	m_IncludeDirectory = pPath.wstring();
}

Shader ShaderCompiler::CompileShader(
	Shader::Type Type,
	const std::filesystem::path& Path,
	LPCWSTR pEntryPoint,
	const std::vector<DxcDefine>& ShaderDefines) const
{
	auto profileString = ShaderProfileString(Type, m_ShaderModel);

	IDxcBlob* shader = nullptr;
	IDxcBlob* pdb = nullptr;
	std::wstring pdbName;
	Compile(Path, pEntryPoint, profileString.data(), ShaderDefines, &shader, &pdb, pdbName);

	return Shader(Type, shader, pdb, std::move(pdbName));
}

Library ShaderCompiler::CompileLibrary(
	const std::filesystem::path& Path) const
{
	auto profileString = LibraryProfileString(m_ShaderModel);

	IDxcBlob* shader = nullptr;
	IDxcBlob* pdb = nullptr;
	std::wstring pdbName;
	Compile(Path, L"", profileString.data(), {}, &shader, &pdb, pdbName);

	return Library(shader, pdb, std::move(pdbName));
}

std::wstring ShaderCompiler::ShaderProfileString(
	Shader::Type Type,
	D3D_SHADER_MODEL ShaderModel) const
{
	std::wstring profileString;
	switch (Type)
	{
	case Shader::Type::Vertex:			profileString = L"vs_"; break;
	case Shader::Type::Hull:			profileString = L"hs_"; break;
	case Shader::Type::Domain:			profileString = L"ds_"; break;
	case Shader::Type::Geometry:		profileString = L"gs_"; break;
	case Shader::Type::Pixel:			profileString = L"ps_"; break;
	case Shader::Type::Compute:			profileString = L"cs_"; break;
	case Shader::Type::Amplification:	profileString = L"as_"; break;
	case Shader::Type::Mesh:			profileString = L"ms_"; break;
	}

	switch (ShaderModel)
	{
	case D3D_SHADER_MODEL_6_3: profileString += L"6_3"; break;
	case D3D_SHADER_MODEL_6_4: profileString += L"6_4"; break;
	case D3D_SHADER_MODEL_6_5: profileString += L"6_5"; break;
	case D3D_SHADER_MODEL_6_6: profileString += L"6_6"; break;
	}

	return profileString;
}

std::wstring ShaderCompiler::LibraryProfileString(
	D3D_SHADER_MODEL ShaderModel) const
{
	std::wstring profileString = L"lib_";
	switch (ShaderModel)
	{
	case D3D_SHADER_MODEL_6_3: profileString += L"6_3"; break;
	case D3D_SHADER_MODEL_6_4: profileString += L"6_4"; break;
	case D3D_SHADER_MODEL_6_5: profileString += L"6_5"; break;
	case D3D_SHADER_MODEL_6_6: profileString += L"6_6"; break;
	}

	return profileString;
}

void ShaderCompiler::Compile(
	const std::filesystem::path& Path,
	LPCWSTR pEntryPoint,
	LPCWSTR pProfile,
	const std::vector<DxcDefine>& ShaderDefines,
	_Outptr_result_maybenull_ IDxcBlob** ppDxcBlob,
	_Outptr_result_maybenull_ IDxcBlob** ppPdbBlob,
	std::wstring& PdbName) const
{
	*ppDxcBlob = nullptr;
	*ppPdbBlob = nullptr;

	// https://developer.nvidia.com/dx12-dos-and-donts
	LPCWSTR arguments[] =
	{
		// Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible This allows for the compiler to do a better job at optimizing texture accesses.
		// We have seen frame rate improvements of > 1 % when toggling this flag on.
		L"-all_resources_bound",
#ifdef _DEBUG
		L"-WX",				// Warnings as errors
		L"-Zi",				// Debug info
		L"-Qembed_debug",	// Embed debug info into the shader
		L"-Od",				// Disable optimization
#else
		L"-O3",				// Optimization level 3
#endif
		// Add include directory
		L"-I", m_IncludeDirectory.data()
	};

	// Build arguments
	ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
	ThrowIfFailed(m_DxcUtils->BuildArguments(
		Path.c_str(),
		pEntryPoint,
		pProfile,
		arguments, ARRAYSIZE(arguments),
		ShaderDefines.data(), ShaderDefines.size(),
		dxcCompilerArgs.ReleaseAndGetAddressOf()));

	ComPtr<IDxcBlobEncoding> source;
	UINT32 codePage = CP_ACP;

	ThrowIfFailed(m_DxcUtils->LoadFile(
		Path.c_str(),
		&codePage,
		source.ReleaseAndGetAddressOf()));

	BOOL sourceKnown = FALSE;
	UINT sourceCodePage = 0;
	ThrowIfFailed(source->GetEncoding(&sourceKnown, &codePage));

	DxcBuffer dxcBuffer = {};
	dxcBuffer.Ptr = source->GetBufferPointer();
	dxcBuffer.Size = source->GetBufferSize();
	dxcBuffer.Encoding = sourceCodePage;

	ComPtr<IDxcResult> dxcResult;
	ThrowIfFailed(m_DxcCompiler->Compile(&dxcBuffer,
		dxcCompilerArgs->GetArguments(), dxcCompilerArgs->GetCount(),
		m_DxcIncludeHandler.Get(),
		IID_PPV_ARGS(dxcResult.ReleaseAndGetAddressOf())));

	HRESULT status;
	dxcResult->GetStatus(&status);
	if (FAILED(status))
	{
		ComPtr<IDxcBlobUtf8> errors;
		ComPtr<IDxcBlobUtf16> outputName;
		if (SUCCEEDED(dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), outputName.ReleaseAndGetAddressOf())))
		{
			OutputDebugStringA(std::bit_cast<char*>(errors->GetBufferPointer()));
			throw std::exception("Failed to compile shader");
		}
	}

	dxcResult->GetResult(ppDxcBlob);
	if (dxcResult->HasOutput(DXC_OUT_PDB))
	{
		ComPtr<IDxcBlobUtf16> pPdbName;
		dxcResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(ppPdbBlob), pPdbName.ReleaseAndGetAddressOf());
		{
			PdbName = pPdbName->GetStringPointer();
		}
	}
}
