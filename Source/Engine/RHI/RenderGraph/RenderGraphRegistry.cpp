#include "RenderGraphRegistry.h"
#include "RenderGraph.h"

namespace RHI
{
	auto RenderGraphRegistry::CreateRootSignature(D3D12RootSignature&& RootSignature) -> RgResourceHandle
	{
		return RootSignatureRegistry.Add(std::forward<D3D12RootSignature>(RootSignature));
	}

	auto RenderGraphRegistry::CreatePipelineState(D3D12PipelineState&& PipelineState) -> RgResourceHandle
	{
		return PipelineStateRegistry.Add(std::forward<D3D12PipelineState>(PipelineState));
	}

	auto RenderGraphRegistry::CreateRaytracingPipelineState(D3D12RaytracingPipelineState&& RaytracingPipelineState) -> RgResourceHandle
	{
		return RaytracingPipelineStateRegistry.Add(std::forward<D3D12RaytracingPipelineState>(RaytracingPipelineState));
	}

	D3D12RootSignature* RenderGraphRegistry::GetRootSignature(RgResourceHandle Handle)
	{
		return RootSignatureRegistry.GetResource(Handle);
	}

	D3D12PipelineState* RenderGraphRegistry::GetPipelineState(RgResourceHandle Handle)
	{
		return PipelineStateRegistry.GetResource(Handle);
	}

	D3D12RaytracingPipelineState* RenderGraphRegistry::GetRaytracingPipelineState(RgResourceHandle Handle)
	{
		return RaytracingPipelineStateRegistry.GetResource(Handle);
	}

	template<typename T>
	bool ClearCache(bool ShouldClear, T& Cache, size_t NewSize)
	{
		if (ShouldClear)
		{
			T().swap(Cache);
			Cache.resize(NewSize);
		}
		return ShouldClear;
	}

	void RenderGraphRegistry::RealizeResources(RenderGraph* Graph, D3D12Device* Device)
	{
		this->Graph = Graph;

		// TODO: Add buffers support, I haven't had the need to use buffers, so code for them are not implemented yet.
		/*
		 * If a graph's resource has changed (e.g. the number of resources mismatches than what the registry cache has),
		 * it will trigger reallocation for all resources, this ensures the resources needed by the pass are
		 * allocated correctly (good index)
		 *
		 * TODO: Maybe a more robust way to cache resources and detect changes to render passes?
		 */

		if (ClearCache(BufferCache.size() != Graph->Buffers.size(), BufferCache, Graph->Buffers.size()))
		{
			BufferDescTable.clear();
		}
		if (ClearCache(TextureCache.size() != Graph->Textures.size(), TextureCache, Graph->Textures.size()))
		{
			TextureDescTable.clear();
		}
		ClearCache(RtvCache.size() != Graph->Rtvs.size(), RtvCache, Graph->Rtvs.size());
		ClearCache(DsvCache.size() != Graph->Dsvs.size(), DsvCache, Graph->Dsvs.size());
		ClearCache(SrvCache.size() != Graph->Srvs.size(), SrvCache, Graph->Srvs.size());
		ClearCache(UavCache.size() != Graph->Uavs.size(), UavCache, Graph->Uavs.size());

		// This is used to check to see if any view associated with the texture needs to be updated in case if texture is dirty
		// The view does not check for this, so do it here manually
		std::unordered_set<RgResourceHandle> TextureDirtyHandles;

		for (size_t i = 0; i < Graph->Textures.size(); ++i)
		{
			const auto&		 RgTexture = Graph->Textures[i];
			RgResourceHandle Handle	   = RgTexture.Handle;
			assert(!Handle.IsImported());

			bool TextureDirty = false;
			auto Iter		  = TextureDescTable.find(Handle);
			if (Iter == TextureDescTable.end())
			{
				TextureDirty = true;
			}
			else
			{
				TextureDirty = Iter->second != RgTexture.Desc;
			}
			TextureDescTable[Handle] = RgTexture.Desc;

			if (!TextureDirty)
			{
				continue;
			}

			TextureDirtyHandles.insert(Handle);
			const RgTextureDesc& Desc = RgTexture.Desc;

			D3D12_RESOURCE_DESC				 ResourceDesc  = {};
			D3D12_RESOURCE_FLAGS			 ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
			std::optional<D3D12_CLEAR_VALUE> ClearValue	   = std::nullopt;

			if (Desc.AllowRenderTarget)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				ClearValue = CD3DX12_CLEAR_VALUE(Desc.Format, Desc.ClearValue.Color);
			}
			if (Desc.AllowDepthStencil)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
				ClearValue = CD3DX12_CLEAR_VALUE(Desc.Format, Desc.ClearValue.DepthStencil.Depth, Desc.ClearValue.DepthStencil.Stencil);
			}
			if (Desc.AllowUnorderedAccess)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			switch (Desc.Type)
			{
			case RgTextureType::Texture2D:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, 1, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			case RgTextureType::Texture2DArray:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			case RgTextureType::Texture3D:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, ResourceFlags);
				break;
			case RgTextureType::TextureCube:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(Desc.Format, Desc.Width, Desc.Height, Desc.DepthOrArraySize, Desc.MipLevels, 1, 0, ResourceFlags);
				break;
			}

			TextureCache[i]	  = D3D12Texture(Device->GetLinkedDevice(), ResourceDesc, ClearValue);
			std::wstring Name = std::wstring(RgTexture.Desc.Name.begin(), RgTexture.Desc.Name.end());
			TextureCache[i].GetResource()->SetName(Name.data());
		}

		for (size_t i = 0; i < Graph->Rtvs.size(); ++i)
		{
			const auto& RgView = Graph->Rtvs[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			if (RgView.Desc.AssociatedResource.IsImported())
			{
				RtvCache[i] = D3D12RenderTargetView(
					Device->GetLinkedDevice(),
					GetImportedResource(RgView.Desc.AssociatedResource),
					RgView.Desc.Rtv.sRGB,
					TextureSubresource::RTV(RgView.Desc.Rtv.MipSlice, RgView.Desc.Rtv.ArraySlice, RgView.Desc.Rtv.ArraySize));
			}
			else
			{
				RtvCache[i] = D3D12RenderTargetView(
					Device->GetLinkedDevice(),
					Get<D3D12Texture>(RgView.Desc.AssociatedResource),
					RgView.Desc.Rtv.sRGB,
					TextureSubresource::RTV(RgView.Desc.Rtv.MipSlice, RgView.Desc.Rtv.ArraySlice, RgView.Desc.Rtv.ArraySize));
			}
		}

		for (size_t i = 0; i < Graph->Dsvs.size(); ++i)
		{
			const auto& RgView = Graph->Dsvs[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			if (RgView.Desc.AssociatedResource.IsImported())
			{
				DsvCache[i] = D3D12DepthStencilView(
					Device->GetLinkedDevice(),
					GetImportedResource(RgView.Desc.AssociatedResource),
					TextureSubresource::DSV(RgView.Desc.Rtv.MipSlice, RgView.Desc.Rtv.ArraySlice, RgView.Desc.Rtv.ArraySize));
			}
			else
			{
				DsvCache[i] = D3D12DepthStencilView(
					Device->GetLinkedDevice(),
					Get<D3D12Texture>(RgView.Desc.AssociatedResource),
					TextureSubresource::DSV(RgView.Desc.Rtv.MipSlice, RgView.Desc.Rtv.ArraySlice, RgView.Desc.Rtv.ArraySize));
			}
		}

		for (size_t i = 0; i < Graph->Srvs.size(); ++i)
		{
			const auto& RgView = Graph->Srvs[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			switch (RgView.Desc.Type)
			{
			case RgViewType::BufferSrv:
			{
				SrvCache[i] = D3D12ShaderResourceView(
					Device->GetLinkedDevice(),
					Get<D3D12Buffer>(RgView.Desc.AssociatedResource),
					RgView.Desc.BufferSrv.Raw,
					RgView.Desc.BufferSrv.FirstElement,
					RgView.Desc.BufferSrv.NumElements);
			}
			break;

			case RgViewType::TextureSrv:
			{
				SrvCache[i] = D3D12ShaderResourceView(
					Device->GetLinkedDevice(),
					Get<D3D12Texture>(RgView.Desc.AssociatedResource),
					RgView.Desc.TextureSrv.sRGB,
					TextureSubresource::SRV(RgView.Desc.TextureSrv.MipSlice, TextureSubresource::AllMipLevels, 0, TextureSubresource::AllArraySlices));
			}
			break;

			default:
				assert(false && "Invalid Srv");
			}
		}

		for (size_t i = 0; i < Graph->Uavs.size(); ++i)
		{
			const auto& RgView = Graph->Uavs[i];
			if (!IsViewDirty(RgView) && !TextureDirtyHandles.contains(RgView.Desc.AssociatedResource))
			{
				continue;
			}

			switch (RgView.Desc.Type)
			{
			case RgViewType::BufferUav:
			{
				UavCache[i] = D3D12UnorderedAccessView(
					Device->GetLinkedDevice(),
					Get<D3D12Buffer>(RgView.Desc.AssociatedResource),
					RgView.Desc.BufferUav.NumElements,
					RgView.Desc.BufferUav.CounterOffsetInBytes);
			}
			break;

			case RgViewType::TextureUav:
			{
				UavCache[i] = D3D12UnorderedAccessView(
					Device->GetLinkedDevice(),
					Get<D3D12Texture>(RgView.Desc.AssociatedResource),
					TextureSubresource::UAV(RgView.Desc.TextureUav.MipSlice, RgView.Desc.TextureUav.ArraySlice, -1));
			}
			break;

			default:
				assert(false && "Invalid Uav");
			}
		}
	}

	D3D12Texture* RenderGraphRegistry::GetImportedResource(RgResourceHandle Handle)
	{
		assert(Handle.IsImported());
		return Graph->ImportedTextures[Handle.Id];
	}

	bool RenderGraphRegistry::IsViewDirty(const RgView& View)
	{
		RgResourceHandle Handle = View.Handle; // View handle

		bool ViewDirty;
		auto Iter = ViewDescTable.find(Handle);
		if (Iter == ViewDescTable.end())
		{
			ViewDirty = true;
		}
		else
		{
			ViewDirty = Iter->second != View.Desc;
		}
		ViewDescTable[Handle] = View.Desc;
		return ViewDirty;
	}
} // namespace RHI
