#include "RenderGraphScheduler.h"
#include "RenderPass.h"

auto RenderGraphScheduler::CreateTexture(std::string_view Name, const RGTextureDesc& Desc) -> RenderResourceHandle
{
	if (Desc.Resolution == ETextureResolution::Render || Desc.Resolution == ETextureResolution::Viewport)
	{
		assert(Desc.Type == ETextureType::Texture2D);
	}

	RGTexture& Texture = Textures.emplace_back();
	Texture.Name		= Name;
	Texture.Handle		= TextureHandle;
	Texture.Desc		= Desc;

	++TextureHandle.Id;

	CurrentRenderPass->Write(Texture.Handle);
	return Texture.Handle;
}

auto RenderGraphScheduler::CreateRenderTarget(const RGRenderTargetDesc& Desc) -> RenderResourceHandle
{
	RenderTargets.emplace_back(Desc);
	RenderResourceHandle Handle = RenderTargetHandle;

	++RenderTargetHandle.Id;

	return Handle;
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
