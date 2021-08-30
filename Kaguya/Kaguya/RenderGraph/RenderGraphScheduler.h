#pragma once
#include "RenderGraphCommon.h"

class RenderPass;

struct RHIBuffer
{
	RenderResourceHandle Handle;
	RGBufferDesc		 Desc;
};

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
		BufferHandle.Type	 = ERGResourceType::Buffer;
		BufferHandle.State	 = ERGHandleState::Dirty;
		BufferHandle.Version = 0;
		BufferHandle.Id		 = 0;

		TextureHandle.Type	  = ERGResourceType::Texture;
		TextureHandle.State	  = ERGHandleState::Dirty;
		TextureHandle.Version = 0;
		TextureHandle.Id	  = 0;
	}

	[[nodiscard]] auto CreateBuffer(std::string_view Name, const RGBufferDesc& Desc) -> RenderResourceHandle;
	[[nodiscard]] auto CreateTexture(std::string_view Name, const RGTextureDesc& Desc) -> RenderResourceHandle;
	[[nodiscard]] auto Read(RenderResourceHandle Resource) -> RenderResourceHandle;
	[[nodiscard]] auto Write(RenderResourceHandle Resource) -> RenderResourceHandle;

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
		assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);
		if (Resource.Type == ERGResourceType::Buffer)
		{
		}
		return Textures[Resource.Id].Desc.AllowUnorderedAccess();
	}

	const std::string& GetTextureName(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id >= 0 && Resource.Id < Textures.size());
		return TextureNames[Resource.Id];
	}

private:
	// Sets the current render pass to be scheduled
	void SetCurrentRenderPass(RenderPass* pRenderPass) { CurrentRenderPass = pRenderPass; }

private:
	friend class RenderGraph;
	friend class RenderGraphRegistry;

	// Handles have a 1 : 1 mapping to resource containers

	RenderPass* CurrentRenderPass;

	RenderResourceHandle	 BufferHandle;
	std::vector<RHIBuffer>	 Buffers;
	std::vector<std::string> BufferNames;

	RenderResourceHandle	 TextureHandle;
	std::vector<RHITexture>	 Textures;
	std::vector<std::string> TextureNames;
};
