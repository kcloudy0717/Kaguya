#pragma once
#include "d3dx12.h"

struct DXILLibrary
{
	DXILLibrary(
		_In_ const D3D12_SHADER_BYTECODE& Library,
		_In_ const std::vector<std::wstring>& Symbols)
		: Library(Library)
		, Symbols(Symbols)
	{

	}

	D3D12_SHADER_BYTECODE		Library;
	std::vector<std::wstring>	Symbols;
};

struct HitGroup
{
	HitGroup(
		_In_opt_ LPCWSTR pHitGroupName,
		_In_opt_ LPCWSTR pAnyHitSymbol,
		_In_opt_ LPCWSTR pClosestHitSymbol,
		_In_opt_ LPCWSTR pIntersectionSymbol)
		: HitGroupName(pHitGroupName ? pHitGroupName : L"")
		, AnyHitSymbol(pAnyHitSymbol ? pAnyHitSymbol : L"")
		, ClosestHitSymbol(pClosestHitSymbol ? pClosestHitSymbol : L"")
		, IntersectionSymbol(pIntersectionSymbol ? pIntersectionSymbol : L"")
	{

	}

	std::wstring HitGroupName;
	std::wstring AnyHitSymbol;
	std::wstring ClosestHitSymbol;
	std::wstring IntersectionSymbol;
};

struct RootSignatureAssociation
{
	RootSignatureAssociation(
		_In_ ID3D12RootSignature* pRootSignature,
		_In_ const std::vector<std::wstring>& Symbols)
		: pRootSignature(pRootSignature)
		, Symbols(Symbols)
	{

	}

	ID3D12RootSignature*		pRootSignature;
	std::vector<std::wstring>	Symbols;
};

class RaytracingPipelineStateBuilder
{
public:
	RaytracingPipelineStateBuilder() noexcept;

	D3D12_STATE_OBJECT_DESC Build();

	void AddLibrary(
		_In_ const D3D12_SHADER_BYTECODE& Library,
		_In_ const std::vector<std::wstring>& Symbols);

	void AddHitGroup(
		_In_opt_ LPCWSTR pHitGroupName,
		_In_opt_ LPCWSTR pAnyHitSymbol,
		_In_opt_ LPCWSTR pClosestHitSymbol,
		_In_opt_ LPCWSTR pIntersectionSymbol);

	void AddRootSignatureAssociation(
		_In_ ID3D12RootSignature* pRootSignature,
		_In_ const std::vector<std::wstring>& Symbols);

	void SetGlobalRootSignature(
		_In_ ID3D12RootSignature* pGlobalRootSignature);

	void SetRaytracingShaderConfig(
		_In_ UINT MaxPayloadSizeInBytes,
		_In_ UINT MaxAttributeSizeInBytes);

	void SetRaytracingPipelineConfig(
		_In_ UINT MaxTraceRecursionDepth);

private:
	std::vector<std::wstring> BuildShaderExportList();

private:
	CD3DX12_STATE_OBJECT_DESC				m_Desc;

	std::vector<DXILLibrary>				m_Libraries;
	std::vector<HitGroup>					m_HitGroups;
	std::vector<RootSignatureAssociation>	m_RootSignatureAssociations;
	ID3D12RootSignature*					m_pGlobalRootSignature;
	D3D12_RAYTRACING_SHADER_CONFIG			m_ShaderConfig;
	D3D12_RAYTRACING_PIPELINE_CONFIG		m_PipelineConfig;
};

class RaytracingPipelineState
{
public:
	RaytracingPipelineState() noexcept = default;
	RaytracingPipelineState(
		_In_ ID3D12Device5* pDevice,
		_In_ RaytracingPipelineStateBuilder& Builder);

	ShaderIdentifier GetShaderIdentifier(
		_In_ LPCWSTR pExportName);

	operator ID3D12StateObject* () const { return m_StateObject.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject>			m_StateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;
};
