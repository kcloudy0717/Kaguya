#include "Shader.h"

Shader::Shader(
	RHI_SHADER_TYPE ShaderType,
	Arc<IDxcBlob>	Binary,
	Arc<IDxcBlob>	Pdb)
	: ShaderType(ShaderType)
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

Library::Library(
	Arc<IDxcBlob> Binary,
	Arc<IDxcBlob> Pdb)
	: Binary(Binary)
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
