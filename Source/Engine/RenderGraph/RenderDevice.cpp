#include "RenderDevice.h"

auto RenderDevice::CreateRenderPass(D3D12RenderPass&& RenderPass) -> RenderResourceHandle
{
	return RenderPassRegistry.Add(std::forward<D3D12RenderPass>(RenderPass));
}

auto RenderDevice::CreateRootSignature(std::unique_ptr<D3D12RootSignature>&& RootSignature) -> RenderResourceHandle
{
	return RootSignatureRegistry.Add(std::forward<std::unique_ptr<D3D12RootSignature>>(RootSignature));
}

auto RenderDevice::CreatePipelineState(std::unique_ptr<D3D12PipelineState>&& PipelineState) -> RenderResourceHandle
{
	return PipelineStateRegistry.Add(std::forward<std::unique_ptr<D3D12PipelineState>>(PipelineState));
}

auto RenderDevice::CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState)
	-> RenderResourceHandle
{
	return RaytracingPipelineStateRegistry.Add(
		std::forward<D3D12RaytracingPipelineState>(RaytracingPipelineState));
}

D3D12RenderPass* RenderDevice::GetRenderPass(RenderResourceHandle Handle)
{
	return RenderPassRegistry.GetResource(Handle);
}

D3D12RootSignature* RenderDevice::GetRootSignature(RenderResourceHandle Handle)
{
	return RootSignatureRegistry.GetResource(Handle)->get();
}

D3D12PipelineState* RenderDevice::GetPipelineState(RenderResourceHandle Handle)
{
	return PipelineStateRegistry.GetResource(Handle)->get();
}

D3D12RaytracingPipelineState* RenderDevice::GetRaytracingPipelineState(RenderResourceHandle Handle)
{
	return RaytracingPipelineStateRegistry.GetResource(Handle);
}
