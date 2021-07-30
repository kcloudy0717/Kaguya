#include "RenderGraphRegistry.h"
#include <RenderCore/RenderCore.h>

void RenderGraphRegistry::CreateResources()
{
	// for (const auto& Desc : Scheduler.BufferDescs)
	//{
	//}

	for (const auto& Desc : Scheduler.TextureDescs)
	{
		D3D12_RESOURCE_DESC	 ResourceDesc  = {};
		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

		if (Desc.Flags & TextureFlag_AllowRenderTarget)
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (Desc.Flags & TextureFlag_AllowDepthStencil)
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (Desc.Flags & TextureFlag_AllowUnorderedAccess)
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		switch (Desc.TextureType)
		{
		case ETextureType::Texture2D:
			ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				Desc.Format,
				Desc.Width,
				Desc.Height,
				1,
				Desc.MipLevels,
				1,
				0,
				ResourceFlags);
			break;
		case ETextureType::Texture2DArray:
			ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				Desc.Format,
				Desc.Width,
				Desc.Height,
				Desc.DepthOrArraySize,
				Desc.MipLevels,
				1,
				0,
				ResourceFlags);
			break;
		case ETextureType::Texture3D:
			ResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(
				Desc.Format,
				Desc.Width,
				Desc.Height,
				Desc.DepthOrArraySize,
				Desc.MipLevels,
				ResourceFlags);
			break;
		case ETextureType::TextureCube:
			ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
				Desc.Format,
				Desc.Width,
				Desc.Height,
				Desc.DepthOrArraySize,
				Desc.MipLevels,
				1,
				0,
				ResourceFlags);
			break;
		default:
			break;
		}

		Texture& Texture =
			Textures.emplace_back(RenderCore::pAdapter->GetDevice(), ResourceDesc, Desc.OptimizedClearValue);
		ShaderViews& ShaderViews = TextureShaderViews.emplace_back();
		ShaderViews.SRVs.resize(Texture.GetNumSubresources());
		if (Desc.Flags & TextureFlag_AllowUnorderedAccess)
		{
			ShaderViews.UAVs.resize(Texture.GetNumSubresources());
		}
	}
}

const ShaderResourceView& RenderGraphRegistry::GetTextureSRV(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/)
{
	Texture& Texture = GetTexture(Handle);

	UINT SubresourceIndex = Texture.GetSubresourceIndex(OptArraySlice, OptMipSlice, OptPlaneSlice);

	auto& ShaderViews = TextureShaderViews[Handle.Id];
	if (!ShaderViews.SRVs[SubresourceIndex].Descriptor.IsValid())
	{
		auto SRV = ShaderResourceView(RenderCore::pAdapter->GetDevice());
		Texture.CreateShaderResourceView(SRV);

		ShaderViews.SRVs[SubresourceIndex] = std::move(SRV);
	}

	return ShaderViews.SRVs[SubresourceIndex];
}

const UnorderedAccessView& RenderGraphRegistry::GetTextureUAV(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/)
{
	Texture& Texture = GetTexture(Handle);
	assert(Scheduler.TextureDescs[Handle.Id].Flags & TextureFlag_AllowUnorderedAccess);

	UINT SubresourceIndex = Texture.GetSubresourceIndex(OptArraySlice, OptMipSlice, OptPlaneSlice);

	auto& ShaderViews = TextureShaderViews[Handle.Id];
	if (!ShaderViews.UAVs[SubresourceIndex].Descriptor.IsValid())
	{
		auto UAV = UnorderedAccessView(RenderCore::pAdapter->GetDevice());
		Texture.CreateUnorderedAccessView(UAV, OptArraySlice, OptMipSlice);

		ShaderViews.UAVs[SubresourceIndex] = std::move(UAV);
	}

	return ShaderViews.UAVs[SubresourceIndex];
}

UINT RenderGraphRegistry::GetTextureIndex(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/)
{
	return GetTextureSRV(Handle, OptArraySlice, OptMipSlice, OptPlaneSlice).GetIndex();
}

UINT RenderGraphRegistry::GetRWTextureIndex(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/)
{
	return GetTextureUAV(Handle, OptArraySlice, OptMipSlice, OptPlaneSlice).GetIndex();
}
