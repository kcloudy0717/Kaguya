#include "RenderGraphScheduler.h"
#include "RenderPass.h"

auto RenderGraphScheduler::CreateBuffer(std::string_view Name, const RGBufferDesc& Desc) -> RenderResourceHandle
{
	RHIBuffer& Buffer = Buffers.emplace_back();
	BufferNames.emplace_back(Name.data());
	Buffer.Handle = BufferHandle;
	Buffer.Desc	  = Desc;

	++BufferHandle.Id;

	CurrentRenderPass->Write(Buffer.Handle);
	return Buffer.Handle;
}

auto RenderGraphScheduler::CreateTexture(std::string_view Name, const RGTextureDesc& Desc) -> RenderResourceHandle
{
	if (Desc.Resolution == ETextureResolution::Render || Desc.Resolution == ETextureResolution::Viewport)
	{
		assert(Desc.Type == ETextureType::Texture2D);
	}

	RHITexture& Texture = Textures.emplace_back();
	TextureNames.emplace_back(Name.data());
	Texture.Handle = TextureHandle;
	Texture.Desc   = Desc;

	++TextureHandle.Id;

	CurrentRenderPass->Write(Texture.Handle);
	return Texture.Handle;
}

auto RenderGraphScheduler::Read(RenderResourceHandle Resource) -> RenderResourceHandle
{
	assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);
	CurrentRenderPass->Read(Resource);
	return Resource;
}

auto RenderGraphScheduler::Write(RenderResourceHandle Resource) -> RenderResourceHandle
{
	assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);
	CurrentRenderPass->Write(Resource);
	++Resource.Version;
	return Resource;
}
