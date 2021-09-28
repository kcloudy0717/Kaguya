#pragma once
#include <Core/ObjectPool.h>
#include "RenderGraph/RenderGraphCommon.h"
#include "RenderGraph/RenderGraphScheduler.h"

class RenderResourceManager
{
public:
	RenderResourceManager()
	{
		TextureHandle.Type	  = ERGResourceType::Texture;
		TextureHandle.State	  = ERGHandleState::Dirty;
		TextureHandle.Version = 0;
		TextureHandle.Id	  = 0;
	}
	virtual ~RenderResourceManager() = default;

protected:
	RenderResourceHandle GetNewTextureHandle()
	{
		RenderResourceHandle Handle = TextureHandle;
		++TextureHandle.Id;
		return Handle;
	}

private:
	RenderResourceHandle TextureHandle;
};

class D3D12RenderResourceManager final : public RenderResourceManager
{
public:
	D3D12RenderResourceManager()
		: Buffers(1024)
		, Textures(1024)
	{
	}

	[[nodiscard]] auto CreateBuffer(const RGBufferDesc& Desc) -> RenderResourceHandle {}
	[[nodiscard]] auto CreateTexture(const RGTextureDesc& Desc) -> RenderResourceHandle {}

private:
	ObjectPool<D3D12Buffer>	 Buffers;
	ObjectPool<D3D12Texture> Textures;
};

class VulkanRenderResourceManager final : public RenderResourceManager
{
public:
	VulkanRenderResourceManager()
		: Buffers(1024)
		, Textures(1024)
	{
	}

	[[nodiscard]] auto CreateBuffer(const RGBufferDesc& Desc) -> RenderResourceHandle {}
	[[nodiscard]] auto CreateTexture(const RGTextureDesc& Desc) -> RenderResourceHandle {}

private:
	ObjectPool<VulkanBuffer>  Buffers;
	ObjectPool<VulkanTexture> Textures;
};
