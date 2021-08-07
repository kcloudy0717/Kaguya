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

class RootSignatureRegistry : public RenderRegistry<RootSignatureRegistry, RootSignature>
{
public:
	RootSignatureRegistry()
		: RenderRegistry(ERGResourceType::RootSignature)
	{
	}
};

class PipelineStateRegistry : public RenderRegistry<PipelineStateRegistry, PipelineState>
{
public:
	PipelineStateRegistry()
		: RenderRegistry(ERGResourceType::PipelineState)
	{
	}
};

class RaytracingPipelineStateRegistry : public RenderRegistry<RaytracingPipelineStateRegistry, RaytracingPipelineState>
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
	[[nodiscard]] auto CreateRootSignature(RootSignature&& RootSignature) -> RenderResourceHandle;
	[[nodiscard]] auto CreatePipelineState(PipelineState&& PipelineState) -> RenderResourceHandle;
	[[nodiscard]] auto CreateRaytracingPipelineState(RaytracingPipelineState&& RaytracingPipelineState)
		-> RenderResourceHandle;

	const RootSignature&		   GetRootSignature(RenderResourceHandle Handle) const;
	const PipelineState&		   GetPipelineState(RenderResourceHandle Handle) const;
	const RaytracingPipelineState& GetRaytracingPipelineState(RenderResourceHandle Handle) const;

private:
	RootSignatureRegistry			RootSignatureRegistry;
	PipelineStateRegistry			PipelineStateRegistry;
	RaytracingPipelineStateRegistry RaytracingPipelineStateRegistry;
};
