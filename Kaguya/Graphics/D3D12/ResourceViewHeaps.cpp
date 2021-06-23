#include "pch.h"
#include "ResourceViewHeaps.h"

template<size_t Size>
inline Descriptor AllocateDescriptor(
	DescriptorHeap&				 DescriptorHeap,
	concurrent_pool<void, Size>& DescriptorHeapIndexPool)
{
	auto index = static_cast<UINT>(DescriptorHeapIndexPool.allocate());
	return DescriptorHeap.At(index);
}

template<size_t Size>
inline void ReleaseDescriptor(Descriptor& Descriptor, concurrent_pool<void, Size>& DescriptorHeapIndexPool)
{
	if (Descriptor.IsValid())
	{
		DescriptorHeapIndexPool.free(Descriptor.Index);
		Descriptor = {};
	}
}

ResourceViewHeaps::ResourceViewHeaps(_In_ ID3D12Device* pDevice)
{
	m_ResourceDescriptorHeap =
		DescriptorHeap(pDevice, NumResourceDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_SamplerDescriptorHeap = DescriptorHeap(pDevice, NumSamplerDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_RenderTargetDescriptorHeap =
		DescriptorHeap(pDevice, NumRenderTargetDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_DepthStencilDescriptorHeap =
		DescriptorHeap(pDevice, NumDepthStencilDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#if _DEBUG
	m_ResourceDescriptorHeap->SetName(L"Resource Descriptor Heap");
	m_SamplerDescriptorHeap->SetName(L"Sampler Descriptor Heap");
	m_RenderTargetDescriptorHeap->SetName(L"Render Target Descriptor Heap");
	m_DepthStencilDescriptorHeap->SetName(L"Depth Stencil Descriptor Heap");
#endif
}

Descriptor ResourceViewHeaps::AllocateResourceView()
{
	return AllocateDescriptor(m_ResourceDescriptorHeap, m_ResourceDescriptorIndexPool);
}

Descriptor ResourceViewHeaps::AllocateSampler()
{
	return AllocateDescriptor(m_SamplerDescriptorHeap, m_SamplerDescriptorIndexPool);
}

Descriptor ResourceViewHeaps::AllocateRenderTargetView()
{
	return AllocateDescriptor(m_RenderTargetDescriptorHeap, m_RenderTargetDescriptorIndexPool);
}

Descriptor ResourceViewHeaps::AllocateDepthStencilView()
{
	return AllocateDescriptor(m_DepthStencilDescriptorHeap, m_DepthStencilDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseResourceView(_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_ResourceDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseSampler(_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_SamplerDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseRenderTargetView(_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_RenderTargetDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseDepthStencilView(_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_DepthStencilDescriptorIndexPool);
}
