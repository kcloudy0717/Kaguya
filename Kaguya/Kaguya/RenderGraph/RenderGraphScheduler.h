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
		TextureHandle.Type	  = ERGResourceType::Texture;
		TextureHandle.State	  = ERGHandleState::Dirty;
		TextureHandle.Version = 0;
		TextureHandle.Id	  = 0;
	}

	RenderResourceHandle CreateTexture(ETextureResolution TextureResolution, const RGTextureDesc& Desc);

	RenderResourceHandle Read(RenderResourceHandle Resource);

	RenderResourceHandle Write(RenderResourceHandle Resource);

	bool AllowRenderTarget(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id >= 0 && Resource.Id < Textures.size());
		return Textures[Resource.Id].Desc.AllowRenderTarget();
	}

	bool AllowDepthStencil(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id >= 0 && Resource.Id < Textures.size());
		return Textures[Resource.Id].Desc.AllowDepthStencil();
	}

	bool AllowUnorderedAccess(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id >= 0 && Resource.Id < Textures.size());
		return Textures[Resource.Id].Desc.AllowUnorderedAccess();
	}

private:
	// Sets the current render pass to be scheduled
	void SetCurrentRenderPass(RenderPass* pRenderPass) { CurrentRenderPass = pRenderPass; }

private:
	friend class RenderGraph;
	friend class RenderGraphRegistry;

	// Handles have a 1 : 1 mapping to resource containers

	RenderPass* CurrentRenderPass;

	RenderResourceHandle	TextureHandle;
	std::vector<RHITexture> Textures;
};
