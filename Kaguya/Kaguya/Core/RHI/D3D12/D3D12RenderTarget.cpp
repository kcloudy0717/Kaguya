#include "D3D12RenderTarget.h"

D3D12RenderTarget::D3D12RenderTarget(D3D12LinkedDevice* Parent, const D3D12RenderTargetDesc& Desc)
	: D3D12LinkedDeviceChild(Parent)
	, Desc(Desc)
{
	RenderTargetArray = Parent->GetRtvAllocator().Allocate(Desc.NumRenderTargets);

	for (UINT i = 0; i < Desc.NumRenderTargets; ++i)
	{
		RenderTargetViews[i] = RenderTargetArray[i];

		Desc.RenderTargets[i]
			->CreateRenderTargetView(RenderTargetViews[i], std::nullopt, std::nullopt, std::nullopt, Desc.sRGB[i]);
	}
	if (Desc.DepthStencil)
	{
		DepthStencilArray = Parent->GetDsvAllocator().Allocate(1);
		DepthStencilView  = DepthStencilArray[0];

		Desc.DepthStencil->CreateDepthStencilView(DepthStencilView);
	}
}

D3D12RenderTarget::~D3D12RenderTarget()
{
}
