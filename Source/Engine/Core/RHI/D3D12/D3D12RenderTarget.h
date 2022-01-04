#pragma once
#include "D3D12Common.h"
#include "D3D12DescriptorHeap.h"

class D3D12Texture;

struct D3D12RenderTargetDesc
{
	void AddRenderTarget(D3D12Texture* RenderTarget, bool sRGB)
	{
		RenderTargets[NumRenderTargets] = RenderTarget;
		this->sRGB[NumRenderTargets]	= sRGB;
		NumRenderTargets++;
	}
	void SetDepthStencil(D3D12Texture* DepthStencil)
	{
		this->DepthStencil = DepthStencil;
	}

	UINT		  NumRenderTargets										= 0;
	D3D12Texture* RenderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	bool		  sRGB[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]			= {};
	D3D12Texture* DepthStencil											= nullptr;
};

class D3D12RenderTarget : public D3D12LinkedDeviceChild
{
public:
	D3D12RenderTarget() noexcept = default;
	explicit D3D12RenderTarget(
		D3D12LinkedDevice*			 Parent,
		const D3D12RenderTargetDesc& Desc);

	D3D12RenderTarget(const D3D12RenderTarget&) = delete;
	D3D12RenderTarget& operator=(const D3D12RenderTarget&) = delete;

	D3D12RenderTarget(D3D12RenderTarget&&) = default;
	D3D12RenderTarget& operator=(D3D12RenderTarget&&) = default;

	[[nodiscard]] D3D12Texture*				   GetTextureAt(UINT Index) const noexcept;
	[[nodiscard]] D3D12Texture*				   GetDepthStencilTexture() const noexcept;
	[[nodiscard]] D3D12_CLEAR_VALUE			   GetClearValueAt(UINT Index) const noexcept;
	[[nodiscard]] D3D12_CLEAR_VALUE			   GetDepthStencilClearValue() const noexcept;
	[[nodiscard]] UINT						   GetNumRenderTargets() const noexcept;
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetViewPtr() noexcept;
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE* GetDepthStencilViewPtr() noexcept;

private:
	D3D12RenderTargetDesc Desc = {};

	D3D12DescriptorArray		RenderTargetArray;
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

	D3D12DescriptorArray		DepthStencilArray;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = {};
};
