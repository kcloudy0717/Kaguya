#include "pch.h"
#include "ResourceViewHeaps.h"

template<size_t Size>
inline Descriptor AllocateDescriptor(DescriptorHeap& DescriptorHeap, ThreadSafePool<void, Size>& DescriptorHeapIndexPool)
{
	auto index = static_cast<UINT>(DescriptorHeapIndexPool.allocate());
	return DescriptorHeap.At(index);
}

template<size_t Size>
inline void ReleaseDescriptor(Descriptor& Descriptor, ThreadSafePool<void, Size>& DescriptorHeapIndexPool)
{
	if (Descriptor.IsValid())
	{
		DescriptorHeapIndexPool.free(Descriptor.Index);
		Descriptor = {};
	}
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

void ResourceViewHeaps::ReleaseResourceView(
	_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_ResourceDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseSampler(
	_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_SamplerDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseRenderTargetView(
	_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_RenderTargetDescriptorIndexPool);
}

void ResourceViewHeaps::ReleaseDepthStencilView(
	_In_ Descriptor& Descriptor)
{
	ReleaseDescriptor(Descriptor, m_DepthStencilDescriptorIndexPool);
}
