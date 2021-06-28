#pragma once
#include "Descriptor.h"
#include "DescriptorHeap.h"

class ResourceViewHeaps
{
public:
	enum
	{
		NumResourceDescriptors	   = 4096,
		NumSamplerDescriptors	   = 2048,
		NumRenderTargetDescriptors = 512,
		NumDepthStencilDescriptors = 512
	};

	ResourceViewHeaps(_In_ ID3D12Device* pDevice);

	// clang-format off
	template <typename TViewDesc> DescriptorHeap& GetDescriptorHeap();
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return RenderTargetDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return DepthStencilDescriptorHeap; }
	// clang-format on

	DescriptorHeap& GetResourceDescriptorHeap() noexcept { return ResourceDescriptorHeap; }
	DescriptorHeap& GetSamplerDescriptorHeap() noexcept { return SamplerDescriptorHeap; }
	DescriptorHeap& GetRenderTargetDescriptorHeap() noexcept { return RenderTargetDescriptorHeap; }
	DescriptorHeap& GetDepthStencilDescriptorHeap() noexcept { return DepthStencilDescriptorHeap; }

private:
	DescriptorHeap ResourceDescriptorHeap;
	DescriptorHeap SamplerDescriptorHeap;
	DescriptorHeap RenderTargetDescriptorHeap;
	DescriptorHeap DepthStencilDescriptorHeap;
};
