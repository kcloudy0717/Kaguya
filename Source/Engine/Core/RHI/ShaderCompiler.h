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

struct ShaderCompileOptions
{
	ShaderCompileOptions(std::wstring_view EntryPoint)
		: EntryPoint(EntryPoint)
	{
	}

	void SetDefine(std::wstring_view Define, std::wstring_view Value) { Defines[Define] = Value; }

	std::wstring_view EntryPoint;

	std::map<std::wstring_view, std::wstring_view> Defines;
};

struct ShaderCompilationResult
{
	IDxcBlob* Binary;

	std::wstring PdbName;
	IDxcBlob*	 Pdb;

	DxcShaderHash ShaderHash;
};

class Shader
{
public:
	Shader() noexcept = default;
	Shader(EShaderType ShaderType, const ShaderCompilationResult& Result) noexcept
		: ShaderType(ShaderType)
		, Binary(Result.Binary)
		, PdbName(Result.PdbName)
		, Pdb(Result.Pdb)
		, ShaderHash(Result.ShaderHash)
	{
		AftermathShaderDatabase::AddShader(Binary, Pdb);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Binary->GetBufferPointer(),
									  .BytecodeLength  = Binary->GetBufferSize() };
	}

	[[nodiscard]] DxcShaderHash GetShaderHash() const noexcept { return ShaderHash; }

private:
	EShaderType						 ShaderType;
	Microsoft::WRL::ComPtr<IDxcBlob> Binary;
	std::wstring					 PdbName;
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb;

	DxcShaderHash ShaderHash = {};
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
	Library(const ShaderCompilationResult& Result) noexcept
		: Binary(Result.Binary)
		, PdbName(std::move(Result.PdbName))
		, Pdb(Result.Pdb)
		, ShaderHash(Result.ShaderHash)
	{
		AftermathShaderDatabase::AddShader(Binary, Pdb);
	}

	operator D3D12_SHADER_BYTECODE() const
	{
		return D3D12_SHADER_BYTECODE{ .pShaderBytecode = Binary->GetBufferPointer(),
									  .BytecodeLength  = Binary->GetBufferSize() };
	}

	[[nodiscard]] DxcShaderHash GetShaderHash() const noexcept { return ShaderHash; }

private:
	Microsoft::WRL::ComPtr<IDxcBlob> Binary;
	std::wstring					 PdbName;
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb;

	DxcShaderHash ShaderHash = {};
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

	[[nodiscard]] Shader CompileShader(
		EShaderType					 ShaderType,
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Library CompileLibrary(const std::filesystem::path& Path) const;

	//[[nodiscard]] Shader SpirVCodeGen(
	//	VkDevice					  Device,
	//	EShaderType					  ShaderType,
	//	const std::filesystem::path& Path,
	//	std::wstring_view			  EntryPoint,
	//	const std::vector<DxcDefine>& ShaderDefines) const;

private:
	[[nodiscard]] std::wstring GetShaderModelString() const;

	[[nodiscard]] std::wstring ShaderProfileString(EShaderType ShaderType) const;

	[[nodiscard]] std::wstring LibraryProfileString() const;

	[[nodiscard]] ShaderCompilationResult Compile(
		const std::filesystem::path&  Path,
		std::wstring_view			  EntryPoint,
		std::wstring_view			  Profile,
		const std::vector<DxcDefine>& ShaderDefines) const;

private:
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   Compiler3;
	Microsoft::WRL::ComPtr<IDxcUtils>		   Utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
	EShaderModel							   ShaderModel;
};
