#pragma once
#include <dxcapi.h>
#include <d3d12shader.h>

class Shader
{
public:
	enum class Type
	{
		Unknown,
		Vertex,
		Hull,
		Domain,
		Geometry,
		Pixel,
		Compute,
		Mesh
	};

	Shader() noexcept = default;
	Shader(
		Type Type,
		_In_ IDxcBlob* DxcBlob,
		_In_ ID3D12ShaderReflection* ShaderReflection) noexcept
		: m_Type(Type)
		, m_DxcBlob(DxcBlob)
		, m_ShaderReflection(ShaderReflection)
	{

	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = m_DxcBlob->GetBufferPointer(),
			.BytecodeLength = m_DxcBlob->GetBufferSize() };
	}

private:
	Type m_Type;
	Microsoft::WRL::ComPtr<IDxcBlob> m_DxcBlob;
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> m_ShaderReflection;
};

/*
	A DXIL library can be seen similarly as a regular DLL,
	which contains compiled code that can be accessed using a number of exported symbols.
	In the case of the raytracing pipeline, such symbols correspond to the names of the functions
	implementing the shader programs.
*/
class Library
{
public:
	Library() noexcept = default;
	Library(
		_In_ IDxcBlob* DxcBlob,
		_In_ ID3D12LibraryReflection* LibraryReflection) noexcept
		: m_DxcBlob(DxcBlob)
		, m_LibraryReflection(LibraryReflection)
	{

	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = m_DxcBlob->GetBufferPointer(),
			.BytecodeLength = m_DxcBlob->GetBufferSize() };
	}

private:
	Microsoft::WRL::ComPtr<IDxcBlob> m_DxcBlob;
	Microsoft::WRL::ComPtr<ID3D12LibraryReflection> m_LibraryReflection;
};

class ShaderCompiler
{
public:
	enum class Profile
	{
		Profile_6_3,
		Profile_6_4,
		Profile_6_5
	};

	ShaderCompiler();

	void SetIncludeDirectory(
		const std::filesystem::path& pPath);

	Shader CompileShader(
		Shader::Type Type,
		const std::filesystem::path& Path,
		LPCWSTR pEntryPoint,
		const std::vector<DxcDefine>& ShaderDefines) const;

	Library CompileLibrary(
		const std::filesystem::path& Path) const;

private:
	std::wstring ShaderProfileString(
		Shader::Type Type,
		ShaderCompiler::Profile Profile) const;

	std::wstring LibraryProfileString(
		ShaderCompiler::Profile Profile) const;

	void Compile(
		const std::filesystem::path& Path,
		LPCWSTR pEntryPoint,
		LPCWSTR pProfile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** ppDxcBlob) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_DxcCompiler;
	Microsoft::WRL::ComPtr<IDxcUtils> m_DxcUtils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_DxcIncludeHandler;
	std::array<WCHAR, MAX_PATH> m_IncludeDirectory;
};
