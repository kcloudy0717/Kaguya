#pragma once
#include "RenderGraphCommon.h"

template<typename TDerived, typename T>
class RenderRegistry
{
public:
	RenderRegistry(ERGResourceType Type)
	{
		Handle.Type	   = Type;
		Handle.State   = ERGHandleState::Dirty;
		Handle.Version = 0;
		Handle.Id	   = 0;
	}

	// static TDerived& Get()
	//{
	//	static TDerived Instance;
	//	return Instance;
	//}

	template<typename... TArgs>
	[[nodiscard]] auto Construct(TArgs&&... Args) -> RenderResourceHandle
	{
		RenderResourceHandle NewHandle = Handle;
		Registry.emplace_back(std::forward<TArgs>(Args)...);

		++Handle.Id;

		return NewHandle;
	}

	[[nodiscard]] auto GetResource(RenderResourceHandle Handle) const -> const T&
	{
		assert(this->Handle.Type == Handle.Type);
		return Registry[Handle.Id];
	}

protected:
	RenderResourceHandle Handle;
	std::vector<T>		 Registry;
};

class RenderPassRegistry : public RenderRegistry<RenderPassRegistry, D3D12RenderPass>
{
public:
	RenderPassRegistry()
		: RenderRegistry(ERGResourceType::RenderPass)
	{
	}
};

class RootSignatureRegistry : public RenderRegistry<RootSignatureRegistry, D3D12RootSignature>
{
public:
	RootSignatureRegistry()
		: RenderRegistry(ERGResourceType::RootSignature)
	{
	}
};

class PipelineStateRegistry : public RenderRegistry<PipelineStateRegistry, D3D12PipelineState>
{
public:
	PipelineStateRegistry()
		: RenderRegistry(ERGResourceType::PipelineState)
	{
	}
};

class RaytracingPipelineStateRegistry
	: public RenderRegistry<RaytracingPipelineStateRegistry, D3D12RaytracingPipelineState>
{
public:
	RaytracingPipelineStateRegistry()
		: RenderRegistry(ERGResourceType::RaytracingPipelineState)
	{
	}
};

class RenderDevice
{
public:
	[[nodiscard]] auto CreateRenderPass(D3D12RenderPass&& RenderPass) -> RenderResourceHandle;
	[[nodiscard]] auto CreateRootSignature(D3D12RootSignature&& RootSignature) -> RenderResourceHandle;
	[[nodiscard]] auto CreatePipelineState(D3D12PipelineState&& PipelineState) -> RenderResourceHandle;
	[[nodiscard]] auto CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState)
		-> RenderResourceHandle;

	const D3D12RenderPass&				GetRenderPass(RenderResourceHandle Handle) const;
	const D3D12RootSignature&			GetRootSignature(RenderResourceHandle Handle) const;
	const D3D12PipelineState&			GetPipelineState(RenderResourceHandle Handle) const;
	const D3D12RaytracingPipelineState& GetRaytracingPipelineState(RenderResourceHandle Handle) const;

private:
	RenderPassRegistry				RenderPassRegistry;
	RootSignatureRegistry			RootSignatureRegistry;
	PipelineStateRegistry			PipelineStateRegistry;
	RaytracingPipelineStateRegistry RaytracingPipelineStateRegistry;
};
