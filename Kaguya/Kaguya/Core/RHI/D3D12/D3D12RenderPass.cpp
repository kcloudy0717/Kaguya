#include "D3D12RenderPass.h"

D3D12RenderPass::D3D12RenderPass(D3D12Device* Parent, const RenderPassDesc& Desc)
	: D3D12DeviceChild(Parent)
	, Desc(Desc)
{
	bool ValidDepthStencilAttachment = Desc.DepthStencil.IsValid();

	UINT NumRenderTargets = Desc.NumRenderTargets;

	for (UINT i = 0; i < NumRenderTargets; ++i)
	{
		const auto& Attachment = Desc.RenderTargets[i];

		RenderTargetFormats.RTFormats[i] = Attachment.Format;
	}
	RenderTargetFormats.NumRenderTargets = NumRenderTargets;

	if (ValidDepthStencilAttachment)
	{
		DepthStencilFormat = Desc.DepthStencil.Format;
	}
}

D3D12RenderPass::~D3D12RenderPass()
{
}
