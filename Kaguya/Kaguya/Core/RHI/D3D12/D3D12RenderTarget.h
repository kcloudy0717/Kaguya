#pragma once
#include "D3D12Common.h"

class D3D12RenderTarget : public D3D12LinkedDeviceChild
{
public:
	D3D12RenderTarget(D3D12LinkedDevice* Parent, const RenderTargetDesc& Desc);
	~D3D12RenderTarget();

	[[nodiscard]] RenderTargetDesc GetDesc() const noexcept { return Desc; }

	RenderTargetDesc Desc = {};

	D3D12DescriptorArray		RenderTargetArray;
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViews[8] = {};
	UINT						NumRenderTargets	 = 0;

	D3D12DescriptorArray		DepthStencilArray;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView;
};
