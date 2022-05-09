#pragma once
#include "Shader.h"
#include "System/System.h"

DECLARE_LOG_CATEGORY(Dxc);

class DxcException : public Exception
{
public:
	DxcException(std::string_view File, int Line, HRESULT ErrorCode)
		: Exception(File, Line)
		, ErrorCode(ErrorCode)
	{
	}

	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	const HRESULT ErrorCode;
};

struct ShaderCompilationResult
{
	Microsoft::WRL::ComPtr<IDxcBlob> Binary;
	std::wstring					 PdbName;
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb;
	DxcShaderHash					 ShaderHash;
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

	[[nodiscard]] Shader CompileHS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompileDS(
		const std::filesystem::path& Path,
		const ShaderCompileOptions&	 Options) const;

	[[nodiscard]] Shader CompileGS(
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
	Microsoft::WRL::ComPtr<IDxcCompiler3>	   Compiler3;
	Microsoft::WRL::ComPtr<IDxcUtils>		   Utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
	RHI_SHADER_MODEL						   ShaderModel;
};
