#pragma once
#include "RenderGraphCommon.h"

class RenderPass;

class RenderGraphScheduler : public RenderGraphChild
{
public:
	RenderGraphScheduler(RenderGraph* Parent)
		: RenderGraphChild(Parent)
	{
	}

	RenderResourceHandle CreateBuffer(const RGBufferDesc& Desc);

	RenderResourceHandle CreateTexture(ETextureResolution TextureResolution, const RGTextureDesc& Desc);

	RenderResourceHandle Read(RenderResourceHandle Resource);

private:
	// Sets the current render pass to be scheduled
	void SetCurrentRenderPass(RenderPass* pRenderPass) { CurrentRenderPass = pRenderPass; }

private:
	friend class RenderGraph;
	friend class RenderGraphRegistry;

	RenderPass* CurrentRenderPass;

	UINT64							  BufferId = 0;
	std::vector<RenderResourceHandle> BufferHandles;
	std::vector<RGBufferDesc>		  BufferDescs;

	UINT64							  TextureId = 0;
	std::vector<RenderResourceHandle> TextureHandles;
	std::vector<RGTextureDesc>		  TextureDescs;
};
