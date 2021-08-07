#include "RenderDevice.h"

auto RenderDevice::CreateRootSignature(RootSignature&& RootSignature) -> RenderResourceHandle
{
	return RootSignatureRegistry.Construct(std::move(RootSignature));
}

auto RenderDevice::CreatePipelineState(PipelineState&& PipelineState) -> RenderResourceHandle
{
	return PipelineStateRegistry.Construct(std::move(PipelineState));
}

auto RenderDevice::CreateRaytracingPipelineState(RaytracingPipelineState&& RaytracingPipelineState)
	-> RenderResourceHandle
{
	return RaytracingPipelineStateRegistry.Construct(std::move(RaytracingPipelineState));
}

const RootSignature& RenderDevice::GetRootSignature(RenderResourceHandle Handle) const
{
	return RootSignatureRegistry.GetResource(Handle);
}

const PipelineState& RenderDevice::GetPipelineState(RenderResourceHandle Handle) const
{
	return PipelineStateRegistry.GetResource(Handle);
}

const RaytracingPipelineState& RenderDevice::GetRaytracingPipelineState(RenderResourceHandle Handle) const
{
	return RaytracingPipelineStateRegistry.GetResource(Handle);
}
