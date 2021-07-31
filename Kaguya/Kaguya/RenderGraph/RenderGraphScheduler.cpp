#include "RenderGraphScheduler.h"
#include "RenderPass.h"

RenderResourceHandle RenderGraphScheduler::CreateTexture(
	ETextureResolution	 TextureResolution,
	const RGTextureDesc& Desc)
{
	if (TextureResolution == ETextureResolution::Render || TextureResolution == ETextureResolution::Viewport)
	{
		assert(Desc.TextureType == ETextureType::Texture2D);
	}

	RHITexture& Texture			   = Textures.emplace_back();
	Texture.Handle.Type			   = ERGResourceType::Texture;
	Texture.Handle.State		   = ERGHandleState::Dirty;
	Texture.Handle.Id			   = TextureId++;
	Texture.Desc				   = Desc;
	Texture.Desc.TextureResolution = TextureResolution;

	CurrentRenderPass->Write(Texture.Handle);
	return Texture.Handle;
}

RenderResourceHandle RenderGraphScheduler::Read(RenderResourceHandle Resource)
{
	assert(Resource.Type == ERGResourceType::Buffer || Resource.Type == ERGResourceType::Texture);

	CurrentRenderPass->Read(Resource);

	return Resource;
}
