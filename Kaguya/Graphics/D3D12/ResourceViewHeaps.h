#pragma once
#include "DescriptorHeap.h"

class ResourceViewHeaps
{
public:
	enum
	{
		NumResourceDescriptors = 2048,
		NumSamplerDescriptors = 512,
		NumRenderTargetDescriptors = 512,
		NumDepthStencilDescriptors = 512
	};

	ResourceViewHeaps() noexcept = default;
	explicit ResourceViewHeaps(
		_In_ ID3D12Device* pDevice)
	{
		m_ResourceDescriptorHeap = DescriptorHeap(pDevice, NumResourceDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_SamplerDescriptorHeap = DescriptorHeap(pDevice, NumSamplerDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_RenderTargetDescriptorHeap = DescriptorHeap(pDevice, NumRenderTargetDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_DepthStencilDescriptorHeap = DescriptorHeap(pDevice, NumDepthStencilDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	[[nodiscard]] Descriptor AllocateResourceView();

	[[nodiscard]] Descriptor AllocateRenderTargetView();

	[[nodiscard]] Descriptor AllocateDepthStencilView();

	void ReleaseResourceView(
		_In_ Descriptor Descriptor);

	void ReleaseRenderTargetView(
		_In_ Descriptor Descriptor);

	void ReleaseDepthStencilView(
		_In_ Descriptor Descriptor);

	DescriptorHeap& ResourceDescriptorHeap() noexcept { return m_ResourceDescriptorHeap; }
	DescriptorHeap& SamplerDescriptorHeap() noexcept { return m_SamplerDescriptorHeap; }
	DescriptorHeap& RenderTargetDescriptorHeap() noexcept { return m_RenderTargetDescriptorHeap; }
	DescriptorHeap& DepthStencilDescriptorHeap() noexcept { return m_DepthStencilDescriptorHeap; }

	const DescriptorHeap& ResourceDescriptorHeap() const noexcept { return m_ResourceDescriptorHeap; }
	const DescriptorHeap& SamplerDescriptorHeap() const noexcept { return m_SamplerDescriptorHeap; }
	const DescriptorHeap& RenderTargetDescriptorHeap() const noexcept { return m_RenderTargetDescriptorHeap; }
	const DescriptorHeap& DepthStencilDescriptorHeap()  const noexcept { return m_DepthStencilDescriptorHeap; }

private:
	DescriptorHeap m_ResourceDescriptorHeap;
	DescriptorHeap m_SamplerDescriptorHeap;
	DescriptorHeap m_RenderTargetDescriptorHeap;
	DescriptorHeap m_DepthStencilDescriptorHeap;

	ThreadSafePool<void, NumResourceDescriptors> m_ResourceDescriptorIndexPool;
	ThreadSafePool<void, NumSamplerDescriptors> m_SamplerDescriptorIndexPool;
	ThreadSafePool<void, NumRenderTargetDescriptors> m_RenderTargetDescriptorIndexPool;
	ThreadSafePool<void, NumDepthStencilDescriptors> m_DepthStencilDescriptorIndexPool;
};
