#include "RenderGraphScheduler.h"
#include "RenderPass.h"

RenderResourceHandle RenderGraphScheduler::CreateBuffer(const RGBufferDesc& Desc)
{
	BufferDescs.push_back(Desc);
	RenderResourceHandle Handle = { .Type = ERGResourceType::Buffer, .Id = BufferId++ };
	CurrentRenderPass->Write(Handle);
	return Handle;
}

RenderResourceHandle RenderGraphScheduler::CreateTexture(RGTextureSize TextureSize, const RGTextureDesc& Desc)
{
	TextureDescs.push_back(Desc);
	RenderResourceHandle Handle = { .Type = ERGResourceType::Texture, .Id = TextureId++ };
	CurrentRenderPass->Write(Handle);
	return Handle;
}

RenderResourceHandle RenderGraphScheduler::Read(RenderResourceHandle Resource)
{
	assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);

	CurrentRenderPass->Read(Resource);

	return Resource;
}
