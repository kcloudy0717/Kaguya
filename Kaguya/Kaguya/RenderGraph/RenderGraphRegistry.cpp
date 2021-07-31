#include "RenderGraphRegistry.h"
#include "RenderGraph.h"
#include <RenderCore/RenderCore.h>

void RenderGraphRegistry::Initialize()
{
	Textures.resize(Scheduler.Textures.size());
	TextureShaderViews.resize(Scheduler.Textures.size());
}

void RenderGraphRegistry::ScheduleResources()
{
	for (size_t i = 0; i < Scheduler.Textures.size(); ++i)
	{
		auto& RHITexture = Scheduler.Textures[i];

		RenderResourceHandle& Handle = RHITexture.Handle;
		RGTextureDesc&		  Desc	 = RHITexture.Desc;

		if (Handle.State == ERGHandleState::Ready)
		{
			continue;
		}
		Handle.State = ERGHandleState::Ready;

		if (Desc.TextureResolution == ETextureResolution::Render)
		{
			auto [w, h] = Scheduler.GetParentRenderGraph()->GetRenderResolution();
			Desc.Width	= w;
			Desc.Height = h;
		}
		else if (Desc.TextureResolution == ETextureResolution::Viewport)
		{
			auto [w, h] = Scheduler.GetParentRenderGraph()->GetViewportResolution();
			Desc.Width	= w;
			Desc.Height = h;
		}

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

		Textures[i]				 = Texture(RenderCore::pAdapter->GetDevice(), ResourceDesc, Desc.OptimizedClearValue);
		ShaderViews& ShaderViews = TextureShaderViews[i];

		if (ShaderViews.SRVs.empty())
		{
			ShaderViews.SRVs.resize(Textures[i].GetNumSubresources());
		}

		if (Desc.Flags & TextureFlag_AllowUnorderedAccess)
		{
			if (ShaderViews.UAVs.empty())
			{
				ShaderViews.UAVs.resize(Textures[i].GetNumSubresources());
			}
		}

		for (auto& SRV : ShaderViews.SRVs)
		{
			if (SRV.Descriptor.IsValid())
			{
				SRV.Descriptor.CreateView(SRV.Desc, Textures[i]);
			}
		}

		for (auto& UAV : ShaderViews.UAVs)
		{
			if (UAV.Descriptor.IsValid())
			{
				UAV.Descriptor.CreateView(UAV.Desc, Textures[i], nullptr);
			}
		}
	}
}

const ShaderResourceView& RenderGraphRegistry::GetTextureSRV(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/)
{
	RenderResourceHandle& HandleRef = Scheduler.Textures[Handle.Id].Handle;

	Texture& Texture = GetTexture(Handle);

	UINT SubresourceIndex = Texture.GetSubresourceIndex(OptArraySlice, OptMipSlice, OptPlaneSlice);

	auto& ShaderViews = TextureShaderViews[Handle.Id];
	if (!ShaderViews.SRVs[SubresourceIndex].Descriptor.IsValid())
	{
		ShaderViews.SRVs[SubresourceIndex] = ShaderResourceView(RenderCore::pAdapter->GetDevice());
		Texture.CreateShaderResourceView(ShaderViews.SRVs[SubresourceIndex]);
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
	assert(Scheduler.Textures[Handle.Id].Desc.Flags & TextureFlag_AllowUnorderedAccess);

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
