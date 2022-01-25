#include "D3D12RenderTarget.h"
#include "D3D12LinkedDevice.h"
#include "D3D12Resource.h"

D3D12RenderTarget::D3D12RenderTarget(
	D3D12LinkedDevice*			 Parent,
	const D3D12RenderTargetDesc& Desc)
	: D3D12LinkedDeviceChild(Parent)
	, Desc(Desc)
{
	RenderTargetArray = Parent->GetRtvAllocator().Allocate(Desc.NumRenderTargets);

	for (UINT i = 0; i < Desc.NumRenderTargets; ++i)
	{
		RenderTargetViews[i] = RenderTargetArray[i];
		Desc.RenderTargets[i]->CreateRenderTargetView(RenderTargetViews[i], std::nullopt, std::nullopt, std::nullopt, Desc.sRGB[i]);
	}
	if (Desc.DepthStencil)
	{
		DepthStencilArray = Parent->GetDsvAllocator().Allocate(1);
		DepthStencilView  = DepthStencilArray[0];
		Desc.DepthStencil->CreateDepthStencilView(DepthStencilView);
	}
}

D3D12Texture* D3D12RenderTarget::GetTextureAt(UINT Index) const noexcept
{
	return Desc.RenderTargets[Index];
}

D3D12Texture* D3D12RenderTarget::GetDepthStencilTexture() const noexcept
{
	return Desc.DepthStencil;
}

D3D12_CLEAR_VALUE D3D12RenderTarget::GetClearValueAt(UINT Index) const noexcept
{
	return Desc.RenderTargets[Index]->GetClearValue();
}

D3D12_CLEAR_VALUE D3D12RenderTarget::GetDepthStencilClearValue() const noexcept
{
	return Desc.DepthStencil->GetClearValue();
}

UINT D3D12RenderTarget::GetNumRenderTargets() const noexcept
{
	return Desc.NumRenderTargets;
}

D3D12_CPU_DESCRIPTOR_HANDLE* D3D12RenderTarget::GetRenderTargetViewPtr() noexcept
{
	return RenderTargetArray.IsValid() ? RenderTargetViews : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE* D3D12RenderTarget::GetDepthStencilViewPtr() noexcept
{
	return DepthStencilArray.IsValid() ? &DepthStencilView : nullptr;
}
