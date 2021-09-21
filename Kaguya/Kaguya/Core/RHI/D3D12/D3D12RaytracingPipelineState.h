#pragma once
#include "D3D12Common.h"

struct DxilLibrary
{
	DxilLibrary(const D3D12_SHADER_BYTECODE& Library, const std::vector<std::wstring>& Symbols)
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
		std::wstring_view				 HitGroupName,
		std::optional<std::wstring_view> AnyHitSymbol,
		std::optional<std::wstring_view> ClosestHitSymbol,
		std::optional<std::wstring_view> IntersectionSymbol)
		: HitGroupName(HitGroupName)
		, AnyHitSymbol(AnyHitSymbol ? *AnyHitSymbol : L"")
		, ClosestHitSymbol(ClosestHitSymbol ? *ClosestHitSymbol : L"")
		, IntersectionSymbol(IntersectionSymbol ? *IntersectionSymbol : L"")
	{
	}

	std::wstring HitGroupName;
	std::wstring AnyHitSymbol;
	std::wstring ClosestHitSymbol;
	std::wstring IntersectionSymbol;
};

struct RootSignatureAssociation
{
	RootSignatureAssociation(ID3D12RootSignature* RootSignature, const std::vector<std::wstring>& Symbols)
		: RootSignature(RootSignature)
		, Symbols(Symbols)
	{
	}

	ID3D12RootSignature*	  RootSignature;
	std::vector<std::wstring> Symbols;
};

class RaytracingPipelineStateBuilder
{
public:
	RaytracingPipelineStateBuilder() noexcept;

	D3D12_STATE_OBJECT_DESC Build();

	void AddLibrary(const D3D12_SHADER_BYTECODE& Library, const std::vector<std::wstring>& Symbols);

	void AddHitGroup(
		std::wstring_view				 HitGroupName,
		std::optional<std::wstring_view> AnyHitSymbol,
		std::optional<std::wstring_view> ClosestHitSymbol,
		std::optional<std::wstring_view> IntersectionSymbol);

	void AddRootSignatureAssociation(ID3D12RootSignature* RootSignature, const std::vector<std::wstring>& Symbols);

	void SetGlobalRootSignature(ID3D12RootSignature* GlobalRootSignature);

	void SetRaytracingShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes);

	void SetRaytracingPipelineConfig(UINT MaxTraceRecursionDepth);

private:
	std::vector<std::wstring> BuildShaderExportList();

private:
	CD3DX12_STATE_OBJECT_DESC Desc;

	std::vector<DxilLibrary>			  Libraries;
	std::vector<HitGroup>				  HitGroups;
	std::vector<RootSignatureAssociation> RootSignatureAssociations;
	ID3D12RootSignature*				  GlobalRootSignature;
	D3D12_RAYTRACING_SHADER_CONFIG		  ShaderConfig;
	D3D12_RAYTRACING_PIPELINE_CONFIG	  PipelineConfig;
};

class D3D12RaytracingPipelineState
{
public:
	D3D12RaytracingPipelineState() noexcept = default;
	D3D12RaytracingPipelineState(ID3D12Device5* Device, RaytracingPipelineStateBuilder& Builder);

	operator ID3D12StateObject*() const { return StateObject.Get(); }

	[[nodiscard]] void* GetShaderIdentifier(std::wstring_view ExportName) const;

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject>			StateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> StateObjectProperties;
};
