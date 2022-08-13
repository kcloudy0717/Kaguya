#pragma once
#include <filesystem>
#include "Shader.h"

DECLARE_LOG_CATEGORY(Dxc);

class ExceptionDxc final : public Exception
{
public:
	ExceptionDxc(std::string_view Source, int Line, HRESULT ErrorCode)
		: Exception(Source, Line)
		, ErrorCode(ErrorCode)
	{
	}

private:
	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	HRESULT ErrorCode;
};

struct ShaderCompilationResult
{
	Arc<IDxcBlob> Binary;
	std::wstring  PdbName;
	Arc<IDxcBlob> Pdb;
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

	[[nodiscard]] Shader CompileVS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompilePS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompileCS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompileAS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompileMS(
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
	Arc<IDxcCompiler3>		Compiler3;
	Arc<IDxcUtils>			Utils;
	Arc<IDxcIncludeHandler> DefaultIncludeHandler;
	RHI_SHADER_MODEL		ShaderModel;
};
