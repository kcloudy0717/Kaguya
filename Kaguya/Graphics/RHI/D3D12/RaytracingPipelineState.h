#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "RootSignature.h"
#include "Library.h"
#include "Shader.h"
#include "ShaderTable.h"

//----------------------------------------------------------------------------------------------------
class RaytracingPipelineStateBuilder;

//----------------------------------------------------------------------------------------------------
struct DXILLibrary
{
	DXILLibrary(const Library* pLibrary, const std::vector<std::wstring>& Symbols);

	const Library*					pLibrary;
	const std::vector<std::wstring> Symbols;
};

//----------------------------------------------------------------------------------------------------
struct HitGroup
{
	HitGroup(LPCWSTR pHitGroupName, LPCWSTR pAnyHitSymbol, LPCWSTR pClosestHitSymbol, LPCWSTR pIntersectionSymbol);

	const std::wstring HitGroupName;
	const std::wstring AnyHitSymbol;
	const std::wstring ClosestHitSymbol;
	const std::wstring IntersectionSymbol;
};

//----------------------------------------------------------------------------------------------------
struct RootSignatureAssociation
{
	RootSignatureAssociation(const RootSignature* pRootSignature, const std::vector<std::wstring>& Symbols);

	const RootSignature*			pRootSignature;
	const std::vector<std::wstring> Symbols;
};

//----------------------------------------------------------------------------------------------------
class RaytracingPipelineState
{
public:
	RaytracingPipelineState() = default;
	RaytracingPipelineState(ID3D12Device5* pDevice, RaytracingPipelineStateBuilder& Builder);

	operator auto() const { return m_StateObject.Get(); }

	ShaderIdentifier GetShaderIdentifier(LPCWSTR pExportName);

	const RootSignature*								pGlobalRootSignature;
private:
	Microsoft::WRL::ComPtr<ID3D12StateObject>			m_StateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_StateObjectProperties;
};