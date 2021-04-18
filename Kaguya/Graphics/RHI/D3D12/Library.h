#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <d3d12shader.h>

/*
	A DXIL library can be seen similarly as a regular DLL,
	which contains compiled code that can be accessed using a number of exported symbols.
	In the case of the raytracing pipeline, such symbols correspond to the names of the functions
	implementing the shader programs.
*/
class Library
{
public:
	Library() = default;
	Library(Microsoft::WRL::ComPtr<IDxcBlob> DxcBlob, Microsoft::WRL::ComPtr<ID3D12LibraryReflection> LibraryReflection)
		: m_DxcBlob(DxcBlob)
		, m_LibraryReflection(LibraryReflection)
	{

	}

	inline auto GetDxcBlob() const { return m_DxcBlob.Get(); }
	inline D3D12_SHADER_BYTECODE GetD3DShaderBytecode() const { return { m_DxcBlob->GetBufferPointer(), m_DxcBlob->GetBufferSize() }; }
private:
	Microsoft::WRL::ComPtr<IDxcBlob>				m_DxcBlob;
	Microsoft::WRL::ComPtr<ID3D12LibraryReflection> m_LibraryReflection;
};