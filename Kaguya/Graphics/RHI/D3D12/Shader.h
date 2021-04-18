#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <d3d12shader.h>

struct ShaderIdentifier
{
	BYTE Data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

class Shader
{
public:
	enum class Type
	{
		Unknown,
		Vertex,
		Hull,
		Domain,
		Geometry,
		Pixel,
		Compute,
		Mesh
	};

	Shader() = default;
	Shader(Type Type, Microsoft::WRL::ComPtr<IDxcBlob> DxcBlob, Microsoft::WRL::ComPtr<ID3D12ShaderReflection> ShaderReflection)
		: m_Type(Type)
		, m_DxcBlob(DxcBlob)
		, m_ShaderReflection(ShaderReflection)
	{

	}

	inline auto GetType() const { return m_Type; }
	inline auto GetDxcBlob() const { return m_DxcBlob.Get(); }
	inline auto GetShaderReflection() const { return m_ShaderReflection.Get(); }
	inline D3D12_SHADER_BYTECODE GetD3DShaderBytecode() const { return { m_DxcBlob->GetBufferPointer(), m_DxcBlob->GetBufferSize() }; }
private:
	Type m_Type;
	Microsoft::WRL::ComPtr<IDxcBlob> m_DxcBlob;
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> m_ShaderReflection;
};