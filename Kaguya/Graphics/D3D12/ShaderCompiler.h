#pragma once
#include "dxcapi.h"
#include "d3d12shader.h"

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
		Amplification,
		Mesh,
	};

	Shader() noexcept = default;
	Shader(Type Type, IDxcBlob* DxcBlob, IDxcBlob* PdbBlob, std::wstring&& PdbName) noexcept
		: m_DxcBlob(DxcBlob)
		, m_PdbBlob(PdbBlob)
		, m_PdbName(std::move(PdbName))
	{
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = m_DxcBlob->GetBufferPointer(),
									  .BytecodeLength  = m_DxcBlob->GetBufferSize() };
	}

	D3D12_SHADER_BYTECODE GetPdb() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = m_DxcBlob->GetBufferPointer(),
									  .BytecodeLength  = m_DxcBlob->GetBufferSize() };
	}

private:
	Type							 m_Type;
	Microsoft::WRL::ComPtr<IDxcBlob> m_DxcBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> m_PdbBlob;
	std::wstring					 m_PdbName;
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
	Library(IDxcBlob* DxcBlob, IDxcBlob* PdbBlob, std::wstring&& PdbName) noexcept
		: m_DxcBlob(DxcBlob)
		, m_PdbBlob(PdbBlob)
		, m_PdbName(std::move(PdbName))
	{
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = m_DxcBlob->GetBufferPointer(),
									  .BytecodeLength  = m_DxcBlob->GetBufferSize() };
	}

private:
	Microsoft::WRL::ComPtr<IDxcBlob> m_DxcBlob;
	Microsoft::WRL::ComPtr<IDxcBlob> m_PdbBlob;
	std::wstring					 m_PdbName;
};

class ShaderCompiler
{
public:
	ShaderCompiler();

	void SetShaderModel(D3D_SHADER_MODEL ShaderModel);

	void SetIncludeDirectory(const std::filesystem::path& pPath);

	Shader CompileShader(
		Shader::Type				  Type,
		const std::filesystem::path&  Path,
		LPCWSTR						  pEntryPoint,
		const std::vector<DxcDefine>& ShaderDefines) const;

	Library CompileLibrary(const std::filesystem::path& Path) const;

private:
	std::wstring ShaderProfileString(Shader::Type Type, D3D_SHADER_MODEL ShaderModel) const;

	std::wstring LibraryProfileString(D3D_SHADER_MODEL ShaderModel) const;

	void Compile(
		const std::filesystem::path&  Path,
		LPCWSTR						  pEntryPoint,
		LPCWSTR						  pProfile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** ppDxcBlob,
		_Outptr_result_maybenull_ IDxcBlob** ppPdbBlob,
		std::wstring&						 PdbName) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   m_DxcCompiler;
	Microsoft::WRL::ComPtr<IDxcUtils>		   m_DxcUtils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_DxcIncludeHandler;
	D3D_SHADER_MODEL						   m_ShaderModel;
	std::wstring							   m_IncludeDirectory;
};
