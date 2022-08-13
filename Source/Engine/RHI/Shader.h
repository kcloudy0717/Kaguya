#pragma once
#include <map>
#include "D3D12/D3D12Core.h"
#include "dxc/dxcapi.h"
#include "dxc/d3d12shader.h"
#include "Arc.h"

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

class Shader
{
public:
	Shader() noexcept = default;
	Shader(
		RHI_SHADER_TYPE ShaderType,
		Arc<IDxcBlob>	Binary,
		Arc<IDxcBlob>	Pdb);

	[[nodiscard]] void*	 GetPointer() const noexcept;
	[[nodiscard]] size_t GetSize() const noexcept;

protected:
	RHI_SHADER_TYPE ShaderType;
	Arc<IDxcBlob>	Binary;
	Arc<IDxcBlob>	Pdb;
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
		Arc<IDxcBlob> Binary,
		Arc<IDxcBlob> Pdb);

	[[nodiscard]] void*	 GetPointer() const noexcept;
	[[nodiscard]] size_t GetSize() const noexcept;

private:
	Arc<IDxcBlob> Binary;
	Arc<IDxcBlob> Pdb;
};
