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

	void RenderGraphRegistry::RealizeResources(RenderGraph* Graph, D3D12Device* Device)
	{
		ShaderResourceViews.clear();
		UnorderedAccessViews.clear();

		this->Graph = Graph;
		Buffers.resize(Graph->Buffers.size());
		Textures.resize(Graph->Textures.size());
		RenderTargetViews.resize(Graph->RenderTargetViews.size());
		DepthStencilViews.resize(Graph->DepthStencilViews.size());
		ShaderResourceViews.resize(Graph->ShaderResourceViews.size());
		UnorderedAccessViews.resize(Graph->UnorderedAccessViews.size());

		for (size_t i = 0; i < Graph->Textures.size(); ++i)
		{
			auto& RHITexture = Graph->Textures[i];

			RgResourceHandle& Handle = RHITexture.Handle;
			assert(!Handle.IsImported());
			const RgTextureDesc& Desc = RHITexture.Desc;

			if (Handle.State)
			{
				if (!Textures[i].GetResource() || Desc.Width != Textures[i].GetDesc().Width || Desc.Height != Textures[i].GetDesc().Height)
				{
					Handle.State = false;
				}
				else
				{
					continue;
				}
			}
			Handle.State = true;

			D3D12_RESOURCE_DESC	 ResourceDesc  = {};
			D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

			if (Desc.RenderTarget)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			}
			if (Desc.DepthStencil)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			}
			if (Desc.UnorderedAccess)
			{
				ResourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			switch (Desc.Type)
			{
			case RgTextureType::Texture2D:
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
			case RgTextureType::Texture2DArray:
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
			case RgTextureType::Texture3D:
				ResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(
					Desc.Format,
					Desc.Width,
					Desc.Height,
					Desc.DepthOrArraySize,
					Desc.MipLevels,
					ResourceFlags);
				break;
			case RgTextureType::TextureCube:
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

			Textures[i]		  = D3D12Texture(Device->GetDevice(), ResourceDesc, Desc.OptimizedClearValue);
			std::wstring Name = std::wstring(RHITexture.Desc.Name.begin(), RHITexture.Desc.Name.end());
			Textures[i].GetResource()->SetName(Name.data());
		}

		for (size_t i = 0; i < Graph->RenderTargetViews.size(); ++i)
		{
			const auto&		 RgView = Graph->RenderTargetViews[i];
			RgResourceHandle Handle = RgView.Desc.Resource;

			D3D12RenderTargetView& RenderTargetView = RenderTargetViews[i];

			std::optional<UINT> ArraySlice = RgView.Desc.RgRtv.ArraySlice != -1 ? RgView.Desc.RgRtv.ArraySlice : std::optional<UINT>{};
			std::optional<UINT> MipSlice   = RgView.Desc.RgRtv.MipSlice != -1 ? RgView.Desc.RgRtv.MipSlice : std::optional<UINT>{};
			std::optional<UINT> ArraySize  = RgView.Desc.RgRtv.ArraySize != -1 ? RgView.Desc.RgRtv.ArraySize : std::optional<UINT>{};
			if (Handle.IsImported())
			{
				RenderTargetView = D3D12RenderTargetView(Device->GetDevice(), GetImportedResource(Handle), ArraySlice, MipSlice, ArraySize, RgView.Desc.RgRtv.sRGB);
			}
			else
			{
				RenderTargetView = D3D12RenderTargetView(Device->GetDevice(), Get<D3D12Texture>(Handle), ArraySlice, MipSlice, ArraySize, RgView.Desc.RgRtv.sRGB);
			}
		}

		for (size_t i = 0; i < Graph->DepthStencilViews.size(); ++i)
		{
			const auto&		 RgView = Graph->DepthStencilViews[i];
			RgResourceHandle Handle = RgView.Desc.Resource;

			D3D12DepthStencilView& DepthStencilView = DepthStencilViews[i];

			std::optional<UINT> ArraySlice = RgView.Desc.RgDsv.ArraySlice != -1 ? RgView.Desc.RgDsv.ArraySlice : std::optional<UINT>{};
			std::optional<UINT> MipSlice   = RgView.Desc.RgDsv.MipSlice != -1 ? RgView.Desc.RgDsv.MipSlice : std::optional<UINT>{};
			std::optional<UINT> ArraySize  = RgView.Desc.RgDsv.ArraySize != -1 ? RgView.Desc.RgDsv.ArraySize : std::optional<UINT>{};
			if (Handle.IsImported())
			{
				DepthStencilView = D3D12DepthStencilView(Device->GetDevice(), GetImportedResource(Handle), ArraySlice, MipSlice, ArraySize);
			}
			else
			{
				DepthStencilView = D3D12DepthStencilView(Device->GetDevice(), Get<D3D12Texture>(Handle), ArraySlice, MipSlice, ArraySize);
			}
		}

		for (size_t i = 0; i < ShaderResourceViews.size(); ++i)
		{
			const auto& RgSrv = Graph->ShaderResourceViews[i];

			switch (RgSrv.Desc.Type)
			{
			case RgViewType::BufferSrv:
			{
				ShaderResourceViews[i] = D3D12ShaderResourceView(
					Device->GetDevice(),
					Get<D3D12Buffer>(RgSrv.Desc.Resource),
					RgSrv.Desc.BufferSrv.Raw,
					RgSrv.Desc.BufferSrv.FirstElement,
					RgSrv.Desc.BufferSrv.NumElements);
			}
			break;

			case RgViewType::TextureSrv:
			{
				std::optional<UINT> MostDetailedMip = RgSrv.Desc.TextureSrv.MostDetailedMip != -1 ? RgSrv.Desc.TextureSrv.MostDetailedMip : std::optional<UINT>{};
				std::optional<UINT> MipLevels		= RgSrv.Desc.TextureSrv.MipLevels != -1 ? RgSrv.Desc.TextureSrv.MipLevels : std::optional<UINT>{};
				ShaderResourceViews[i]				= D3D12ShaderResourceView(
					 Device->GetDevice(),
					 Get<D3D12Texture>(RgSrv.Desc.Resource),
					 RgSrv.Desc.TextureSrv.sRGB,
					 MostDetailedMip,
					 MipLevels);
			}
			break;

			default:
				assert(false && "Invalid Srv");
			}
		}

		for (size_t i = 0; i < UnorderedAccessViews.size(); ++i)
		{
			const auto& RgUav = Graph->UnorderedAccessViews[i];

			switch (RgUav.Desc.Type)
			{
			case RgViewType::BufferUav:
			{
				UnorderedAccessViews[i] = D3D12UnorderedAccessView(
					Device->GetDevice(),
					Get<D3D12Buffer>(RgUav.Desc.Resource),
					RgUav.Desc.BufferUav.NumElements,
					RgUav.Desc.BufferUav.CounterOffsetInBytes);
			}
			break;

			case RgViewType::TextureUav:
			{
				std::optional<UINT> ArraySlice = RgUav.Desc.TextureUav.ArraySlice != -1 ? RgUav.Desc.TextureUav.ArraySlice : std::optional<UINT>{};
				std::optional<UINT> MipSlice   = RgUav.Desc.TextureUav.MipSlice != -1 ? RgUav.Desc.TextureUav.MipSlice : std::optional<UINT>{};
				UnorderedAccessViews[i]		   = D3D12UnorderedAccessView(
					   Device->GetDevice(),
					   Get<D3D12Texture>(RgUav.Desc.Resource),
					   ArraySlice,
					   MipSlice);
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
} // namespace RHI
