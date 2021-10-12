#include "RenderGraphRegistry.h"
#include "RenderGraph.h"
#include <RenderCore/RenderCore.h>

void RenderGraphRegistry::Initialize()
{
	Textures.resize(Scheduler.Textures.size());
	TextureShaderViews.resize(Scheduler.Textures.size());

	RenderTargets.resize(Scheduler.RenderTargets.size());
}

void RenderGraphRegistry::RealizeResources(RenderGraph& RenderGraph)
{
	for (size_t i = 0; i < Scheduler.Textures.size(); ++i)
	{
		auto& RHITexture = Scheduler.Textures[i];

		RenderResourceHandle& Handle = RHITexture.Handle;
		TextureDesc&		  Desc	 = RHITexture.Desc;

		if (Handle.State == ERGHandleState::Ready)
		{
			continue;
		}
		Handle.State = ERGHandleState::Ready;

		if (Desc.Resolution == ETextureResolution::Render)
		{
			auto [w, h] = RenderGraph.GetRenderResolution();
			Desc.Width	= w;
			Desc.Height = h;
		}
		else if (Desc.Resolution == ETextureResolution::Viewport)
		{
			auto [w, h] = RenderGraph.GetViewportResolution();
			Desc.Width	= w;
			Desc.Height = h;
		}

		D3D12_RESOURCE_DESC	 ResourceDesc  = {};
		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

		if (Desc.AllowRenderTarget())
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (Desc.AllowDepthStencil())
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (Desc.AllowUnorderedAccess())
		{
			ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		switch (Desc.Type)
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

		Textures[i] = D3D12Texture(RenderCore::Device->GetDevice(), ResourceDesc, Desc.OptimizedClearValue);
		ShaderViews& ShaderViews = TextureShaderViews[i];

		if (ShaderViews.SRVs.empty())
		{
			ShaderViews.SRVs.resize(Textures[i].GetNumSubresources());
		}

		if (Desc.AllowUnorderedAccess())
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

	size_t i = 0;
	for (const auto& Desc : Scheduler.RenderTargets)
	{
		D3D12RenderTargetDesc ApiDesc = {};

		for (UINT j = 0; j < Desc.NumRenderTargets; ++j)
		{
			ApiDesc.AddRenderTarget(&GetTexture(Desc.RenderTargets[j]), Desc.sRGB[i]);
		}
		if (Desc.DepthStencil.IsValid())
		{
			ApiDesc.SetDepthStencil(&GetTexture(Desc.DepthStencil));
		}

		RenderTargets[i] = D3D12RenderTarget(RenderCore::Device->GetDevice(), ApiDesc);
	}
}

auto RenderGraphRegistry::GetTexture(RenderResourceHandle Handle) -> D3D12Texture&
{
	assert(Handle.Type == ERGResourceType::Texture);
	assert(Handle.Id < Textures.size());
	return Textures[Handle.Id];
}

auto RenderGraphRegistry::GetRenderTarget(RenderResourceHandle Handle) -> D3D12RenderTarget&
{
	assert(Handle.Type == ERGResourceType::RenderTarget);
	assert(Handle.Id < RenderTargets.size());
	return RenderTargets[Handle.Id];
}

auto RenderGraphRegistry::GetTextureSRV(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/) -> const D3D12ShaderResourceView&
{
	RenderResourceHandle& HandleRef = Scheduler.Textures[Handle.Id].Handle;

	D3D12Texture& Texture = GetTexture(Handle);

	UINT SubresourceIndex = Texture.GetSubresourceIndex(OptArraySlice, OptMipSlice, OptPlaneSlice);

	auto& ShaderViews = TextureShaderViews[Handle.Id];
	if (!ShaderViews.SRVs[SubresourceIndex].Descriptor.IsValid())
	{
		ShaderViews.SRVs[SubresourceIndex] = D3D12ShaderResourceView(RenderCore::Device->GetDevice());
		Texture.CreateShaderResourceView(ShaderViews.SRVs[SubresourceIndex]);
	}

	return ShaderViews.SRVs[SubresourceIndex];
}

auto RenderGraphRegistry::GetTextureUAV(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/) -> const D3D12UnorderedAccessView&
{
	D3D12Texture& Texture = GetTexture(Handle);
	assert(Scheduler.Textures[Handle.Id].Desc.Flags & TextureFlag_AllowUnorderedAccess);

	UINT SubresourceIndex = Texture.GetSubresourceIndex(OptArraySlice, OptMipSlice, OptPlaneSlice);

	auto& ShaderViews = TextureShaderViews[Handle.Id];
	if (!ShaderViews.UAVs[SubresourceIndex].Descriptor.IsValid())
	{
		auto UAV = D3D12UnorderedAccessView(RenderCore::Device->GetDevice());
		Texture.CreateUnorderedAccessView(UAV, OptArraySlice, OptMipSlice);

		ShaderViews.UAVs[SubresourceIndex] = std::move(UAV);
	}

	return ShaderViews.UAVs[SubresourceIndex];
}

auto RenderGraphRegistry::GetTextureIndex(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/) -> UINT
{
	return GetTextureSRV(Handle, OptArraySlice, OptMipSlice, OptPlaneSlice).GetIndex();
}

auto RenderGraphRegistry::GetRWTextureIndex(
	RenderResourceHandle Handle,
	std::optional<UINT>	 OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	 OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	 OptPlaneSlice /*= std::nullopt*/) -> UINT
{
	return GetTextureUAV(Handle, OptArraySlice, OptMipSlice, OptPlaneSlice).GetIndex();
}
