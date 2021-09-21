#include "D3D12RenderTarget.h"

D3D12RenderTarget::D3D12RenderTarget(D3D12LinkedDevice* Parent, const RenderTargetDesc& Desc)
	: D3D12LinkedDeviceChild(Parent)
	, Desc(Desc)
{
	RenderTargetArray = Parent->GetRtvAllocator().Allocate(Desc.NumRenderTargets);

	for (UINT i = 0; i < Desc.NumRenderTargets; ++i)
	{
		RenderTargetViews[i] = RenderTargetArray[i];

		Desc.RenderTargets[i]->As<Experimental::D3D12Texture>()->CreateRenderTargetView(RenderTargetViews[i]);
	}
	if (Desc.DepthStencil)
	{
		DepthStencilArray = Parent->GetDsvAllocator().Allocate(1);
		DepthStencilView  = DepthStencilArray[0];

		Desc.DepthStencil->As<Experimental::D3D12Texture>()->CreateDepthStencilView(DepthStencilView);
	}
}

D3D12RenderTarget::~D3D12RenderTarget()
{
}
