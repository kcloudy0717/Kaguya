#pragma once
#include "d3dx12.h"

struct DXILLibrary
{
	DXILLibrary(_In_ const D3D12_SHADER_BYTECODE& Library, _In_ const std::vector<std::wstring>& Symbols)
		: Library(Library)
		, Symbols(Symbols)
	{
	}

	D3D12_SHADER_BYTECODE	  Library;
	std::vector<std::wstring> Symbols;
};

struct HitGroup
{
	HitGroup(
		std::optional<std::wstring_view> pHitGroupName,
		std::optional<std::wstring_view> pAnyHitSymbol,
		std::optional<std::wstring_view> pClosestHitSymbol,
		std::optional<std::wstring_view> pIntersectionSymbol)
		: HitGroupName(pHitGroupName ? *pHitGroupName : L"")
		, AnyHitSymbol(pAnyHitSymbol ? *pAnyHitSymbol : L"")
		, ClosestHitSymbol(pClosestHitSymbol ? *pClosestHitSymbol : L"")
		, IntersectionSymbol(pIntersectionSymbol ? *pIntersectionSymbol : L"")
	{
	}

	std::wstring HitGroupName;
	std::wstring AnyHitSymbol;
	std::wstring ClosestHitSymbol;
	std::wstring IntersectionSymbol;
};

struct RootSignatureAssociation
{
	RootSignatureAssociation(_In_ ID3D12RootSignature* pRootSignature, _In_ const std::vector<std::wstring>& Symbols)
		: pRootSignature(pRootSignature)
		, Symbols(Symbols)
	{
	}

	ID3D12RootSignature*	  pRootSignature;
	std::vector<std::wstring> Symbols;
};

class RaytracingPipelineStateBuilder
{
public:
	RaytracingPipelineStateBuilder() noexcept;

	D3D12_STATE_OBJECT_DESC Build();

	void AddLibrary(const D3D12_SHADER_BYTECODE& Library, const std::vector<std::wstring>& Symbols);

	void AddHitGroup(
		std::optional<std::wstring_view> pHitGroupName,
		std::optional<std::wstring_view> pAnyHitSymbol,
		std::optional<std::wstring_view> pClosestHitSymbol,
		std::optional<std::wstring_view> pIntersectionSymbol);

	void AddRootSignatureAssociation(ID3D12RootSignature* pRootSignature, const std::vector<std::wstring>& Symbols);

	void SetGlobalRootSignature(ID3D12RootSignature* pGlobalRootSignature);

	void SetRaytracingShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes);

	void SetRaytracingPipelineConfig(UINT MaxTraceRecursionDepth);

private:
	std::vector<std::wstring> BuildShaderExportList();

private:
	CD3DX12_STATE_OBJECT_DESC Desc;

	std::vector<DXILLibrary>			  Libraries;
	std::vector<HitGroup>				  HitGroups;
	std::vector<RootSignatureAssociation> RootSignatureAssociations;
	ID3D12RootSignature*				  GlobalRootSignature;
	D3D12_RAYTRACING_SHADER_CONFIG		  ShaderConfig;
	D3D12_RAYTRACING_PIPELINE_CONFIG	  PipelineConfig;
};

class RaytracingPipelineState
{
public:
	RaytracingPipelineState() noexcept = default;
	RaytracingPipelineState(ID3D12Device5* pDevice, RaytracingPipelineStateBuilder& Builder);

	void* GetShaderIdentifier(std::wstring_view pExportName);

	operator ID3D12StateObject*() const { return StateObject.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject>			StateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> StateObjectProperties;
};
