#pragma once
#include <dxcapi.h>
#include <d3d12shader.h>
#include <wrl/client.h>
#include <filesystem>

#include "Shader.h"
#include "Library.h"

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

	void SetIncludeDirectory(const std::filesystem::path& pPath);

	Shader CompileShader(Shader::Type Type, const std::filesystem::path& Path, LPCWSTR pEntryPoint, const std::vector<DxcDefine>& ShaderDefines) const;
	Library CompileLibrary(const std::filesystem::path& Path) const;
private:
	Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::filesystem::path& Path, LPCWSTR pEntryPoint, LPCWSTR pProfile, const std::vector<DxcDefine>& ShaderDefines) const;
private:
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_DxcCompiler;
	Microsoft::WRL::ComPtr<IDxcUtils> m_DxcUtils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_DxcIncludeHandler;
};