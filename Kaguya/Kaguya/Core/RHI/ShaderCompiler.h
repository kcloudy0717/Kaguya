#pragma once
#include "dxcapi.h"
#include "d3d12shader.h"

class DxcException : public CoreException
{
public:
	DxcException(const char* File, int Line, HRESULT ErrorCode)
		: CoreException(File, Line)
		, ErrorCode(ErrorCode)
	{
	}

	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	const HRESULT ErrorCode;
};

#define VERIFY_DXC_API(expr)                                                                                           \
	{                                                                                                                  \
		HRESULT hr = expr;                                                                                             \
		if (FAILED(hr))                                                                                                \
		{                                                                                                              \
			throw DxcException(__FILE__, __LINE__, hr);                                                                \
		}                                                                                                              \
	}

enum class EShaderModel
{
	ShaderModel_6_5,
	ShaderModel_6_6
};

enum class EShaderType
{
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Compute,
	Amplification,
	Mesh
};

class Shader
{
public:
	Shader() noexcept = default;
	Shader(EShaderType ShaderType, IDxcBlob* Blob, IDxcBlob* PDBBlob, std::wstring PDBName) noexcept
		: ShaderType(ShaderType)
		, Blob(Blob)
		, PDBBlob(PDBBlob)
		, PDBName(std::move(PDBName))
	{
		AftermathShaderDatabase::AddShader(Blob, PDBBlob);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Blob->GetBufferPointer(),
									  .BytecodeLength  = Blob->GetBufferSize() };
	}

	[[nodiscard]] auto GetBufferPointer() -> LPVOID const { return Blob->GetBufferPointer(); }
	[[nodiscard]] auto GetBufferSize() -> SIZE_T const { return Blob->GetBufferSize(); }

private:
	EShaderType						 ShaderType;
	Microsoft::WRL::ComPtr<IDxcBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> PDBBlob;
	std::wstring					 PDBName;
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
	Library(IDxcBlob* Blob, IDxcBlob* PDBBlob, std::wstring PDBName) noexcept
		: Blob(Blob)
		, PDBBlob(PDBBlob)
		, PDBName(std::move(PDBName))
	{
		AftermathShaderDatabase::AddShader(Blob, PDBBlob);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Blob->GetBufferPointer(),
									  .BytecodeLength  = Blob->GetBufferSize() };
	}

private:
	Microsoft::WRL::ComPtr<IDxcBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> PDBBlob;
	std::wstring					 PDBName;
};

class ShaderCompiler
{
public:
	ShaderCompiler()
		: ShaderModel(EShaderModel::ShaderModel_6_5)
	{
	}

	void Initialize();

	void SetShaderModel(EShaderModel ShaderModel) noexcept;

	void SetIncludeDirectory(const std::filesystem::path& Path);

	Shader CompileShader(
		EShaderType					  ShaderType,
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		const std::vector<DxcDefine>& ShaderDefines) const;

	Library CompileLibrary(const std::filesystem::path& Path) const;

	Shader SpirVCodeGen(
		EShaderType					  ShaderType,
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		const std::vector<DxcDefine>& ShaderDefines);

private:
	std::wstring GetShaderModelString() const;

	std::wstring ShaderProfileString(EShaderType ShaderType) const;

	std::wstring LibraryProfileString() const;

	void Compile(
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		std::wstring_view			  Profile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** ppBlob,
		_Outptr_result_maybenull_ IDxcBlob** ppPDBBlob,
		std::wstring&						 PDBName) const;

	void SpirV(
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		std::wstring_view			  Profile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** ppBlob) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   Compiler3;
	Microsoft::WRL::ComPtr<IDxcUtils>		   Utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
	EShaderModel							   ShaderModel;
	std::wstring							   IncludeDirectory;
};
