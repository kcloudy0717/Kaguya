#include "pch.h"
#include "ShaderCompiler.h"

using Microsoft::WRL::ComPtr;

ShaderCompiler::ShaderCompiler()
{
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_DxcCompiler.ReleaseAndGetAddressOf())));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_DxcUtils.ReleaseAndGetAddressOf())));
	ThrowIfFailed(m_DxcUtils->CreateDefaultIncludeHandler(m_DxcIncludeHandler.ReleaseAndGetAddressOf()));
	m_IncludeDirectory = {};
}

void ShaderCompiler::SetIncludeDirectory(
	_In_ const std::filesystem::path& pPath)
{
	wcscpy_s(m_IncludeDirectory.data(), m_IncludeDirectory.size(), pPath.wstring().data());
}

Shader ShaderCompiler::CompileShader(
	_In_ Shader::Type Type,
	_In_ const std::filesystem::path& Path,
	_In_ LPCWSTR pEntryPoint,
	_In_ const std::vector<DxcDefine>& ShaderDefines) const
{
	auto profileString = ShaderProfileString(Type, ShaderCompiler::Profile::Profile_6_6);

	IDxcBlob* dxcBlob = nullptr;
	ID3D12ShaderReflection* shaderReflection = nullptr;

	Compile(Path, pEntryPoint, profileString.data(), ShaderDefines, &dxcBlob);

	DxcBuffer dxcBuffer = {};
	dxcBuffer.Ptr = dxcBlob->GetBufferPointer();
	dxcBuffer.Size = dxcBlob->GetBufferSize();
	dxcBuffer.Encoding = CP_ACP;
	ThrowIfFailed(m_DxcUtils->CreateReflection(&dxcBuffer, IID_PPV_ARGS(&shaderReflection)));

	return Shader(Type, dxcBlob, shaderReflection);
}

Library ShaderCompiler::CompileLibrary(
	_In_ const std::filesystem::path& Path) const
{
	auto profileString = LibraryProfileString(ShaderCompiler::Profile::Profile_6_6);

	IDxcBlob* dxcBlob = nullptr;
	ID3D12LibraryReflection* libraryReflection = nullptr;

	Compile(Path, L"", profileString.data(), {}, &dxcBlob);

	DxcBuffer dxcBuffer = {};
	dxcBuffer.Ptr = dxcBlob->GetBufferPointer();
	dxcBuffer.Size = dxcBlob->GetBufferSize();
	dxcBuffer.Encoding = CP_ACP;
	ThrowIfFailed(m_DxcUtils->CreateReflection(&dxcBuffer, IID_PPV_ARGS(&libraryReflection)));

	return Library(dxcBlob, libraryReflection);
}

std::wstring ShaderCompiler::ShaderProfileString(
	_In_ Shader::Type Type,
	_In_ ShaderCompiler::Profile Profile) const
{
	std::wstring profileString;
	switch (Type)
	{
	case Shader::Type::Vertex:					profileString = L"vs_"; break;
	case Shader::Type::Hull:					profileString = L"hs_"; break;
	case Shader::Type::Domain:					profileString = L"ds_"; break;
	case Shader::Type::Geometry:				profileString = L"gs_"; break;
	case Shader::Type::Pixel:					profileString = L"ps_"; break;
	case Shader::Type::Compute:					profileString = L"cs_"; break;
	case Shader::Type::Mesh:					profileString = L"ms_"; break;
	}

	switch (Profile)
	{
	case ShaderCompiler::Profile::Profile_6_3:	profileString += L"6_3"; break;
	case ShaderCompiler::Profile::Profile_6_4:	profileString += L"6_4"; break;
	case ShaderCompiler::Profile::Profile_6_5:	profileString += L"6_5"; break;
	case ShaderCompiler::Profile::Profile_6_6:	profileString += L"6_6"; break;
	}

	return profileString;
}

std::wstring ShaderCompiler::LibraryProfileString(
	_In_ ShaderCompiler::Profile Profile) const
{
	std::wstring profileString = L"lib_";
	switch (Profile)
	{
	case ShaderCompiler::Profile::Profile_6_3:	profileString += L"6_3"; break;
	case ShaderCompiler::Profile::Profile_6_4:	profileString += L"6_4"; break;
	case ShaderCompiler::Profile::Profile_6_5:	profileString += L"6_5"; break;
	case ShaderCompiler::Profile::Profile_6_6:	profileString += L"6_6"; break;
	}

	return profileString;
}

void ShaderCompiler::Compile(
	_In_ const std::filesystem::path& Path,
	_In_opt_ LPCWSTR pEntryPoint,
	_In_ LPCWSTR pProfile,
	_In_ const std::vector<DxcDefine>& ShaderDefines,
	_Outptr_result_maybenull_ IDxcBlob** ppDxcBlob) const
{
	*ppDxcBlob = nullptr;

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

	ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
	ThrowIfFailed(m_DxcUtils->BuildArguments(Path.c_str(),
		pEntryPoint,
		pProfile,
		arguments, ARRAYSIZE(arguments),
		ShaderDefines.data(), ShaderDefines.size(),
		dxcCompilerArgs.ReleaseAndGetAddressOf()));

	ComPtr<IDxcBlobEncoding> source;
	UINT32 codePage = CP_ACP;

	ThrowIfFailed(m_DxcUtils->LoadFile(Path.c_str(), &codePage, source.ReleaseAndGetAddressOf()));

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

	HRESULT hr;
	if (SUCCEEDED(dxcResult->GetStatus(&hr)))
	{
		if (SUCCEEDED(hr))
		{
			dxcResult->GetResult(ppDxcBlob);
			return;
		}
	}

	ComPtr<IDxcBlobUtf8> errors;
	ComPtr<IDxcBlobUtf16> outputName;
	if (SUCCEEDED(dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), outputName.ReleaseAndGetAddressOf())))
	{
		OutputDebugStringA(std::bit_cast<char*>(errors->GetBufferPointer()));
		throw std::exception("Failed to compile shader");
	}
}
