#include "Shader.h"

Shader::Shader(
	RHI_SHADER_TYPE					 ShaderType,
	DxcShaderHash					 ShaderHash,
	Microsoft::WRL::ComPtr<IDxcBlob> Binary,
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb)
	: ShaderType(ShaderType)
	, ShaderHash(ShaderHash)
	, Binary(Binary)
	, Pdb(Pdb)
{
}

void* Shader::GetPointer() const noexcept
{
	return Binary->GetBufferPointer();
}

size_t Shader::GetSize() const noexcept
{
	return Binary->GetBufferSize();
}

DxcShaderHash Shader::GetShaderHash() const noexcept
{
	return ShaderHash;
}

Library::Library(
	DxcShaderHash					 ShaderHash,
	Microsoft::WRL::ComPtr<IDxcBlob> Binary,
	Microsoft::WRL::ComPtr<IDxcBlob> Pdb)
	: ShaderHash(ShaderHash)
	, Binary(Binary)
	, Pdb(Pdb)
{
}

void* Library::GetPointer() const noexcept
{
	return Binary->GetBufferPointer();
}

size_t Library::GetSize() const noexcept
{
	return Binary->GetBufferSize();
}

DxcShaderHash Library::GetShaderHash() const noexcept
{
	return ShaderHash;
}
