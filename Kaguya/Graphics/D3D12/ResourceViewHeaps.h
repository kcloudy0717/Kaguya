#pragma once
#include "DescriptorHeap.h"

class ResourceViewHeaps
{
public:
	enum
	{
		NumResourceDescriptors = 1000000,
		NumSamplerDescriptors = 2048,
		NumRenderTargetDescriptors = 512,
		NumDepthStencilDescriptors = 512
	};

	ResourceViewHeaps() noexcept = default;
	explicit ResourceViewHeaps(
		_In_ ID3D12Device* pDevice);

	ResourceViewHeaps(ResourceViewHeaps&&) noexcept = default;
	ResourceViewHeaps& operator=(ResourceViewHeaps&&) noexcept = default;

	ResourceViewHeaps(const ResourceViewHeaps&) = delete;
	ResourceViewHeaps& operator=(const ResourceViewHeaps&) = delete;

	[[nodiscard]] Descriptor AllocateResourceView();

	[[nodiscard]] Descriptor AllocateSampler();

	[[nodiscard]] Descriptor AllocateRenderTargetView();

	[[nodiscard]] Descriptor AllocateDepthStencilView();

	void ReleaseResourceView(
		_In_ Descriptor& Descriptor);

	void ReleaseSampler(
		_In_ Descriptor& Descriptor);

	void ReleaseRenderTargetView(
		_In_ Descriptor& Descriptor);

	void ReleaseDepthStencilView(
		_In_ Descriptor& Descriptor);

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

	concurrent_pool<void, NumResourceDescriptors> m_ResourceDescriptorIndexPool;
	concurrent_pool<void, NumSamplerDescriptors> m_SamplerDescriptorIndexPool;
	concurrent_pool<void, NumRenderTargetDescriptors> m_RenderTargetDescriptorIndexPool;
	concurrent_pool<void, NumDepthStencilDescriptors> m_DepthStencilDescriptorIndexPool;
};
