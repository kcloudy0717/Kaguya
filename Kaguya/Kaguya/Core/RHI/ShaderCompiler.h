#pragma once
#include "dxcapi.h"
#include "d3d12shader.h"
#include "D3D12/Aftermath/AftermathShaderDatabase.h"

class DxcException : public Exception
{
public:
	DxcException(const char* File, int Line, HRESULT ErrorCode)
		: Exception(File, Line)
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
	Shader(EShaderType ShaderType, IDxcBlob* Blob, IDxcBlob* PdbBlob, std::wstring PdbName) noexcept
		: ShaderType(ShaderType)
		, Blob(Blob)
		, PdbBlob(PdbBlob)
		, PdbName(std::move(PdbName))
	{
		AftermathShaderDatabase::AddShader(Blob, PdbBlob);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Blob->GetBufferPointer(),
									  .BytecodeLength  = Blob->GetBufferSize() };
	}

	[[nodiscard]] auto GetBufferPointer() const -> LPVOID { return Blob->GetBufferPointer(); }
	[[nodiscard]] auto GetBufferSize() const -> SIZE_T { return Blob->GetBufferSize(); }

	VkShaderModule ShaderModule = VK_NULL_HANDLE;

private:
	EShaderType						 ShaderType;
	Microsoft::WRL::ComPtr<IDxcBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> PdbBlob;
	std::wstring					 PdbName;
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
	Library(IDxcBlob* Blob, IDxcBlob* PdbBlob, std::wstring PdbName) noexcept
		: Blob(Blob)
		, PdbBlob(PdbBlob)
		, PdbName(std::move(PdbName))
	{
		AftermathShaderDatabase::AddShader(Blob, PdbBlob);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Blob->GetBufferPointer(),
									  .BytecodeLength  = Blob->GetBufferSize() };
	}

private:
	Microsoft::WRL::ComPtr<IDxcBlob> Blob;
	Microsoft::WRL::ComPtr<IDxcBlob> PdbBlob;
	std::wstring					 PdbName;
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

	[[nodiscard]] Shader CompileShader(
		EShaderType					  ShaderType,
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		const std::vector<DxcDefine>& ShaderDefines) const;

	[[nodiscard]] Library CompileLibrary(const std::filesystem::path& Path) const;

	[[nodiscard]] Shader SpirVCodeGen(
		VkDevice					  Device,
		EShaderType					  ShaderType,
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		const std::vector<DxcDefine>& ShaderDefines) const;

private:
	[[nodiscard]] std::wstring GetShaderModelString() const;

	[[nodiscard]] std::wstring ShaderProfileString(EShaderType ShaderType) const;

	[[nodiscard]] std::wstring LibraryProfileString() const;

	void Compile(
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		std::wstring_view			  Profile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** OutBlob,
		_Outptr_result_maybenull_ IDxcBlob** OutPdbBlob,
		std::wstring&						 PdbName) const;

	void SpirV(
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		std::wstring_view			  Profile,
		const std::vector<DxcDefine>& ShaderDefines,
		_Outptr_result_maybenull_ IDxcBlob** OutBlob) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   Compiler3;
	Microsoft::WRL::ComPtr<IDxcUtils>		   Utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
	EShaderModel							   ShaderModel;
	std::wstring							   IncludeDirectory;
};
