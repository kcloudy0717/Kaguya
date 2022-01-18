#pragma once
#include <map>
#include <wrl/client.h>
#include "dxcapi.h"
#include "d3d12shader.h"
#include "System/System.h"

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

enum class RHI_SHADER_MODEL
{
	ShaderModel_6_5,
	ShaderModel_6_6
};

enum class RHI_SHADER_TYPE
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
	Shader(
		RHI_SHADER_TYPE				   ShaderType,
		const ShaderCompilationResult& Result) noexcept;

	[[nodiscard]] DxcShaderHash GetShaderHash() const noexcept;
	[[nodiscard]] void*			GetPointer() const noexcept;
	[[nodiscard]] size_t		GetSize() const noexcept;

private:
	RHI_SHADER_TYPE					 ShaderType;
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
	Library(
		const ShaderCompilationResult& Result) noexcept;

	[[nodiscard]] DxcShaderHash GetShaderHash() const noexcept;
	[[nodiscard]] void*			GetPointer() const noexcept;
	[[nodiscard]] size_t		GetSize() const noexcept;

private:
	Microsoft::WRL::ComPtr<IDxcBlob> Binary;
	std::wstring					 PdbName;
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb;

	DxcShaderHash ShaderHash = {};
};

class ShaderCompiler
{
public:
	ShaderCompiler();

	void SetShaderModel(
		RHI_SHADER_MODEL ShaderModel) noexcept;

	[[nodiscard]] Shader CompileShader(
		RHI_SHADER_TYPE				 ShaderType,
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Library CompileLibrary(
		const std::filesystem::path& Path) const;

private:
	[[nodiscard]] std::wstring GetShaderModelString() const;

	[[nodiscard]] std::wstring ShaderProfileString(RHI_SHADER_TYPE ShaderType) const;

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
	RHI_SHADER_MODEL						   ShaderModel;
};
