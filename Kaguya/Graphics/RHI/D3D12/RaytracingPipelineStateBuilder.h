#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <vector>
#include "RaytracingPipelineState.h"

class RaytracingPipelineStateBuilder
{
public:
	RaytracingPipelineStateBuilder();

	D3D12_STATE_OBJECT_DESC Build();

	inline auto GetGlobalRootSignature() const { return m_pGlobalRootSignature; }

	void AddLibrary(const Library* pLibrary, const std::vector<std::wstring>& Symbols);
	void AddHitGroup(LPCWSTR pHitGroupName, LPCWSTR pAnyHitSymbol, LPCWSTR pClosestHitSymbol, LPCWSTR pIntersectionSymbol);
	void AddRootSignatureAssociation(const RootSignature* pRootSignature, const std::vector<std::wstring>& Symbols);
	void SetGlobalRootSignature(const RootSignature* pGlobalRootSignature);

	void SetRaytracingShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes);
	void SetRaytracingPipelineConfig(UINT MaxTraceRecursionDepth);

private:
	std::vector<std::wstring> BuildShaderExportList();

private:
	CD3DX12_STATE_OBJECT_DESC				m_Desc;

	std::vector<DXILLibrary>				m_Libraries;
	std::vector<HitGroup>					m_HitGroups;
	std::vector<RootSignatureAssociation>	m_RootSignatureAssociations;
	const RootSignature*					m_pGlobalRootSignature;
	D3D12_RAYTRACING_SHADER_CONFIG			m_ShaderConfig;
	D3D12_RAYTRACING_PIPELINE_CONFIG		m_PipelineConfig;
};