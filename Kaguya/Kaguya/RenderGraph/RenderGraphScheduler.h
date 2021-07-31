#pragma once
#include "RenderGraphCommon.h"

class RenderPass;

struct RHITexture
{
	RenderResourceHandle Handle;
	RGTextureDesc		 Desc;
};

class RenderGraphScheduler : public RenderGraphChild
{
public:
	RenderGraphScheduler(RenderGraph* Parent)
		: RenderGraphChild(Parent)
	{
	}

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

	UINT64					TextureId = 0;
	std::vector<RHITexture> Textures;
};
