#pragma once
#include "RenderGraphCommon.h"

class RenderPass;

struct RGTexture
{
	std::string			 Name;
	RenderResourceHandle Handle;
	TextureDesc		 Desc;
};

struct RGRenderTargetDesc
{
	void AddRenderTarget(RenderResourceHandle RenderTarget, bool sRGB)
	{
		RenderTargets[NumRenderTargets] = RenderTarget;
		this->sRGB[NumRenderTargets]	= sRGB;
		NumRenderTargets++;
	}
	void SetDepthStencil(RenderResourceHandle DepthStencil) { this->DepthStencil = DepthStencil; }

	RenderResourceHandle RenderPass;
	UINT				 Width;
	UINT				 Height;

	UINT				 NumRenderTargets = 0;
	RenderResourceHandle RenderTargets[8] = {};
	bool				 sRGB[8]		  = {};
	RenderResourceHandle DepthStencil;
};

class RenderGraphScheduler
{
public:
	RenderGraphScheduler()
	{
		TextureHandle.Type		= ERGResourceType::Texture;
		RenderTargetHandle.Type = ERGResourceType::RenderTarget;
	}

	void Reset()
	{
		TextureHandle.State	  = ERGHandleState::Ready;
		TextureHandle.Version = 0;
		TextureHandle.Id	  = 0;

		RenderTargetHandle.State   = ERGHandleState::Ready;
		RenderTargetHandle.Version = 0;
		RenderTargetHandle.Id	   = 0;

		Textures.clear();
		RenderTargets.clear();
	}

	[[nodiscard]] auto CreateBuffer(std::string_view Name, const BufferDesc& Desc) -> RenderResourceHandle;
	[[nodiscard]] auto CreateTexture(std::string_view Name, const TextureDesc& Desc) -> RenderResourceHandle;
	[[nodiscard]] auto CreateRenderTarget(const RGRenderTargetDesc& Desc) -> RenderResourceHandle;

	[[nodiscard]] auto Read(RenderResourceHandle Resource) -> RenderResourceHandle;
	[[nodiscard]] auto Write(RenderResourceHandle Resource) -> RenderResourceHandle;

	[[nodiscard]] bool AllowRenderTarget(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id < Textures.size());
		return Textures[Resource.Id].Desc.AllowRenderTarget();
	}

	[[nodiscard]] bool AllowDepthStencil(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id < Textures.size());
		return Textures[Resource.Id].Desc.AllowDepthStencil();
	}

	[[nodiscard]] bool AllowUnorderedAccess(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);
		// TODO: Buffer
		return Textures[Resource.Id].Desc.AllowUnorderedAccess();
	}

	[[nodiscard]] const std::string& GetTextureName(RenderResourceHandle Resource) const noexcept
	{
		assert(Resource.Type == ERGResourceType::Texture);
		assert(Resource.Id < Textures.size());
		return Textures[Resource.Id].Name;
	}

private:
	// Sets the current render pass to be scheduled
	void SetCurrentRenderPass(RenderPass* RenderPass) { CurrentRenderPass = RenderPass; }

private:
	friend class RenderGraph;
	friend class RenderGraphRegistry;

	// Handles have a 1 : 1 mapping to resource containers

	RenderPass* CurrentRenderPass = nullptr;

	RenderResourceHandle TextureHandle;
	RenderResourceHandle RenderTargetHandle;

	std::vector<RGTexture>			Textures;
	std::vector<RGRenderTargetDesc> RenderTargets;
};
