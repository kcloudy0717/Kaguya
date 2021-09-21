#include "D3D12RenderPass.h"

D3D12RenderPass::D3D12RenderPass(D3D12Device* Parent, const RenderPassDesc& Desc)
	: D3D12DeviceChild(Parent)
	, Desc(Desc)
{
	bool ValidDepthStencilAttachment = Desc.DepthStencil.IsValid();

	UINT NumRenderTargets = Desc.NumRenderTargets;

	for (UINT i = 0; i < NumRenderTargets; ++i)
	{
		const auto& RHIRenderTarget = Desc.RenderTargets[i];

		RenderTargetFormats.RTFormats[i] = ToDxgiFormat(RHIRenderTarget.Format);
	}
	RenderTargetFormats.NumRenderTargets = NumRenderTargets;

	if (ValidDepthStencilAttachment)
	{
		DepthStencilFormat = ToDxgiFormat(Desc.DepthStencil.Format);
	}
}

D3D12RenderPass::~D3D12RenderPass()
{
}
