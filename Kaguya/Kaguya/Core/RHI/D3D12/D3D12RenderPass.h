#pragma once
#include "D3D12Common.h"

class D3D12RenderPass : public D3D12DeviceChild
{
public:
	D3D12RenderPass() noexcept = default;
	D3D12RenderPass(D3D12Device* Parent, const RenderPassDesc& Desc);
	~D3D12RenderPass();

	[[nodiscard]] RenderPassDesc GetDesc() const noexcept { return Desc; }

	RenderPassDesc Desc = {};

	D3D12_RT_FORMAT_ARRAY RenderTargetFormats;
	DXGI_FORMAT			  DepthStencilFormat;
};
