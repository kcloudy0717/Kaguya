#include "RenderGraphScheduler.h"
#include "RenderPass.h"

RenderResourceHandle RenderGraphScheduler::CreateBuffer(const RGBufferDesc& Desc)
{
	RenderResourceHandle& Handle = BufferHandles.emplace_back();
	Handle.Type					 = ERGResourceType::Buffer;
	Handle.State				 = ERGHandleState::Dirty;
	Handle.Id					 = BufferId++;
	BufferDescs.push_back(Desc);
	CurrentRenderPass->Write(Handle);
	return Handle;
}

RenderResourceHandle RenderGraphScheduler::CreateTexture(
	ETextureResolution	 TextureResolution,
	const RGTextureDesc& Desc)
{
	if (TextureResolution == ETextureResolution::Render || TextureResolution == ETextureResolution::Viewport)
	{
		assert(Desc.TextureType == ETextureType::Texture2D);
	}

	RenderResourceHandle& Handle = TextureHandles.emplace_back();
	Handle.Type					 = ERGResourceType::Texture;
	Handle.State				 = ERGHandleState::Dirty;
	Handle.Id					 = TextureId++;

	RGTextureDesc& TextureDesc	  = TextureDescs.emplace_back(Desc);
	TextureDesc.TextureResolution = TextureResolution;

	CurrentRenderPass->Write(Handle);
	return Handle;
}

RenderResourceHandle RenderGraphScheduler::Read(RenderResourceHandle Resource)
{
	assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);

	CurrentRenderPass->Read(Resource);

	return Resource;
}
