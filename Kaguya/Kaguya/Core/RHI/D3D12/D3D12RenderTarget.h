#pragma once
#include "D3D12Common.h"
#include "D3D12DescriptorHeap.h"

struct D3D12RenderTargetDesc
{
	void AddRenderTarget(D3D12Texture* RenderTarget, bool sRGB)
	{
		RenderTargets[NumRenderTargets] = RenderTarget;
		this->sRGB[NumRenderTargets]	= sRGB;
		NumRenderTargets++;
	}
	void SetDepthStencil(D3D12Texture* DepthStencil) { this->DepthStencil = DepthStencil; }

	D3D12RenderPass* RenderPass = nullptr;
	UINT			 Width;
	UINT			 Height;

	UINT		  NumRenderTargets = 0;
	D3D12Texture* RenderTargets[8] = {};
	bool		  sRGB[8]		   = {};
	D3D12Texture* DepthStencil	   = nullptr;
};

class D3D12RenderTarget : public D3D12LinkedDeviceChild
{
public:
	D3D12RenderTarget() noexcept = default;
	D3D12RenderTarget(D3D12LinkedDevice* Parent, const D3D12RenderTargetDesc& Desc);
	~D3D12RenderTarget();

	NONCOPYABLE(D3D12RenderTarget);
	DEFAULTMOVABLE(D3D12RenderTarget);

	D3D12RenderTargetDesc Desc = {};

	D3D12DescriptorArray		RenderTargetArray;
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViews[8] = {};

	D3D12DescriptorArray		DepthStencilArray;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = {};
};
