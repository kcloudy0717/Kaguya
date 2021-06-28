#include "pch.h"
#include "ResourceViewHeaps.h"

ResourceViewHeaps::ResourceViewHeaps(_In_ ID3D12Device* pDevice)
	: ResourceDescriptorHeap(pDevice, NumResourceDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	, SamplerDescriptorHeap(pDevice, NumSamplerDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	, RenderTargetDescriptorHeap(pDevice, NumRenderTargetDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	, DepthStencilDescriptorHeap(pDevice, NumDepthStencilDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
{
#if _DEBUG
	ResourceDescriptorHeap->SetName(L"Resource Descriptor Heap");
	SamplerDescriptorHeap->SetName(L"Sampler Descriptor Heap");
	RenderTargetDescriptorHeap->SetName(L"Render Target Descriptor Heap");
	DepthStencilDescriptorHeap->SetName(L"Depth Stencil Descriptor Heap");
#endif
}
