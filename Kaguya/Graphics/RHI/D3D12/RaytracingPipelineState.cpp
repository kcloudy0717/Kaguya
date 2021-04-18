#include "pch.h"
#include "RaytracingPipelineState.h"
#include "RaytracingPipelineStateBuilder.h"

//----------------------------------------------------------------------------------------------------
DXILLibrary::DXILLibrary(const Library* pLibrary, const std::vector<std::wstring>& Symbols)
	: pLibrary(pLibrary)
	, Symbols(Symbols)
{

}

//----------------------------------------------------------------------------------------------------
HitGroup::HitGroup(LPCWSTR pHitGroupName, LPCWSTR pAnyHitSymbol, LPCWSTR pClosestHitSymbol, LPCWSTR pIntersectionSymbol)
	: HitGroupName(pHitGroupName ? pHitGroupName : L"")
	, AnyHitSymbol(pAnyHitSymbol ? pAnyHitSymbol : L"")
	, ClosestHitSymbol(pClosestHitSymbol ? pClosestHitSymbol : L"")
	, IntersectionSymbol(pIntersectionSymbol ? pIntersectionSymbol : L"")
{

}

//----------------------------------------------------------------------------------------------------
RootSignatureAssociation::RootSignatureAssociation(const RootSignature* pRootSignature, const std::vector<std::wstring>& Symbols)
	: pRootSignature(pRootSignature)
	, Symbols(Symbols)
{

}

//----------------------------------------------------------------------------------------------------
RaytracingPipelineState::RaytracingPipelineState(ID3D12Device5* pDevice, RaytracingPipelineStateBuilder& Builder)
{
	pGlobalRootSignature = Builder.GetGlobalRootSignature();

	auto Desc = Builder.Build();

	ThrowIfFailed(pDevice->CreateStateObject(&Desc, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));
	// Query the state object properties
	ThrowIfFailed(m_StateObject.As(&m_StateObjectProperties));
}

ShaderIdentifier RaytracingPipelineState::GetShaderIdentifier(LPCWSTR pExportName)
{
	ShaderIdentifier ShaderIdentifier = {};
	void* pShaderIdentifier = m_StateObjectProperties->GetShaderIdentifier(pExportName);
	if (!pShaderIdentifier)
	{
		throw std::invalid_argument("Invalid pExportName");
	}

	memcpy(ShaderIdentifier.Data, pShaderIdentifier, sizeof(ShaderIdentifier));
	return ShaderIdentifier;
}