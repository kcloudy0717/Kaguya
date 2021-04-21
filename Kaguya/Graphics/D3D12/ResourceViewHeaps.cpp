#include "pch.h"
#include "ResourceViewHeaps.h"

Descriptor ResourceViewHeaps::AllocateResourceView()
{
	UINT index = m_ResourceDescriptorIndexPool.allocate();
	return m_ResourceDescriptorHeap.At(index);
}

Descriptor ResourceViewHeaps::AllocateSampler()
{
	UINT index = m_SamplerDescriptorIndexPool.allocate();
	return m_SamplerDescriptorHeap.At(index);
}

Descriptor ResourceViewHeaps::AllocateRenderTargetView()
{
	UINT index = m_RenderTargetDescriptorIndexPool.allocate();
	return m_RenderTargetDescriptorHeap.At(index);
}

Descriptor ResourceViewHeaps::AllocateDepthStencilView()
{
	UINT index = m_DepthStencilDescriptorIndexPool.allocate();
	return m_DepthStencilDescriptorHeap.At(index);
}

void ResourceViewHeaps::ReleaseResourceView(
	_In_ Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_ResourceDescriptorIndexPool.free(Descriptor.Index);
	}
}

void ResourceViewHeaps::ReleaseSampler(
	_In_ Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_SamplerDescriptorIndexPool.free(Descriptor.Index);
	}
}

void ResourceViewHeaps::ReleaseRenderTargetView(
	_In_ Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_RenderTargetDescriptorIndexPool.free(Descriptor.Index);
	}
}

void ResourceViewHeaps::ReleaseDepthStencilView(
	_In_ Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_DepthStencilDescriptorIndexPool.free(Descriptor.Index);
	}
}
