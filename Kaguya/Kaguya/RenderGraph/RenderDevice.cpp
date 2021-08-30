#include "RenderDevice.h"

auto RenderDevice::CreateRootSignature(D3D12RootSignature&& RootSignature) -> RenderResourceHandle
{
	return RootSignatureRegistry.Construct(std::move(RootSignature));
}

auto RenderDevice::CreatePipelineState(D3D12PipelineState&& PipelineState) -> RenderResourceHandle
{
	return PipelineStateRegistry.Construct(std::move(PipelineState));
}

auto RenderDevice::CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState)
	-> RenderResourceHandle
{
	return RaytracingPipelineStateRegistry.Construct(std::move(RaytracingPipelineState));
}

const D3D12RootSignature& RenderDevice::GetRootSignature(RenderResourceHandle Handle) const
{
	return RootSignatureRegistry.GetResource(Handle);
}

const D3D12PipelineState& RenderDevice::GetPipelineState(RenderResourceHandle Handle) const
{
	return PipelineStateRegistry.GetResource(Handle);
}

const D3D12RaytracingPipelineState& RenderDevice::GetRaytracingPipelineState(RenderResourceHandle Handle) const
{
	return RaytracingPipelineStateRegistry.GetResource(Handle);
}
