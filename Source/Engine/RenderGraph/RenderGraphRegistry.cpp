#include "RenderGraphRegistry.h"
#include "RenderGraph.h"
#include <RenderCore/RenderCore.h>

auto RenderGraphRegistry::CreateRootSignature(std::unique_ptr<D3D12RootSignature>&& RootSignature) -> RgResourceHandle
{
	return RootSignatureRegistry.Add(std::forward<std::unique_ptr<D3D12RootSignature>>(RootSignature));
}

auto RenderGraphRegistry::CreatePipelineState(std::unique_ptr<D3D12PipelineState>&& PipelineState) -> RgResourceHandle
{
	return PipelineStateRegistry.Add(std::forward<std::unique_ptr<D3D12PipelineState>>(PipelineState));
}

auto RenderGraphRegistry::CreateRaytracingPipelineState(std::unique_ptr<D3D12RaytracingPipelineState>&& RaytracingPipelineState) -> RgResourceHandle
{
	return RaytracingPipelineStateRegistry.Add(std::forward<std::unique_ptr<D3D12RaytracingPipelineState>>(RaytracingPipelineState));
}

D3D12RootSignature* RenderGraphRegistry::GetRootSignature(RgResourceHandle Handle)
{
	return RootSignatureRegistry.GetResource(Handle)->get();
}

D3D12PipelineState* RenderGraphRegistry::GetPipelineState(RgResourceHandle Handle)
{
	return PipelineStateRegistry.GetResource(Handle)->get();
}

D3D12RaytracingPipelineState* RenderGraphRegistry::GetRaytracingPipelineState(RgResourceHandle Handle)
{
	return RaytracingPipelineStateRegistry.GetResource(Handle)->get();
}

void RenderGraphRegistry::RealizeResources(RenderGraph* Graph)
{
	ShaderResourceViews.clear();
	UnorderedAccessViews.clear();

	this->Graph = Graph;
	Buffers.resize(Graph->Buffers.size());
	Textures.resize(Graph->Textures.size());
	RenderTargets.resize(Graph->RenderTargets.size());
	ShaderResourceViews.resize(Graph->ShaderResourceViews.size());
	UnorderedAccessViews.resize(Graph->UnorderedAccessViews.size());

	for (size_t i = 0; i < Graph->Textures.size(); ++i)
	{
		auto& RHITexture = Graph->Textures[i];

		RgResourceHandle&	 Handle = RHITexture.Handle;
		const RgTextureDesc& Desc	= RHITexture.Desc;

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

		Textures[i]		  = D3D12Texture(RenderCore::Device->GetDevice(), ResourceDesc, Desc.OptimizedClearValue);
		std::wstring Name = std::wstring(RHITexture.Name.begin(), RHITexture.Name.end());
		Textures[i].GetResource()->SetName(Name.data());
	}

	for (size_t i = 0; i < Graph->RenderTargets.size(); ++i)
	{
		const auto& RgRt = Graph->RenderTargets[i];

		D3D12RenderTargetDesc ApiDesc = {};

		for (UINT j = 0; j < RgRt.Desc.NumRenderTargets; ++j)
		{
			ApiDesc.AddRenderTarget(&GetTexture(RgRt.Desc.RenderTargets[j]), RgRt.Desc.sRGB[j]);
		}
		if (RgRt.Desc.DepthStencil.IsValid())
		{
			ApiDesc.SetDepthStencil(&GetTexture(RgRt.Desc.DepthStencil));
		}

		RenderTargets[i] = D3D12RenderTarget(RenderCore::Device->GetDevice(), ApiDesc);
	}

	for (size_t i = 0; i < ShaderResourceViews.size(); ++i)
	{
		const auto& RgSrv = Graph->ShaderResourceViews[i];

		switch (RgSrv.Desc.Type)
		{
		case RgViewType::BufferSrv:
		{
			ShaderResourceViews[i] = D3D12ShaderResourceView(
				RenderCore::Device->GetDevice(),
				&GetBuffer(RgSrv.Desc.Resource),
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
				 RenderCore::Device->GetDevice(),
				 &GetTexture(RgSrv.Desc.Resource),
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
				RenderCore::Device->GetDevice(),
				&GetBuffer(RgUav.Desc.Resource),
				RgUav.Desc.BufferUav.NumElements,
				RgUav.Desc.BufferUav.CounterOffsetInBytes);
		}
		break;

		case RgViewType::TextureUav:
		{
			std::optional<UINT> ArraySlice = RgUav.Desc.TextureUav.ArraySlice != -1 ? RgUav.Desc.TextureUav.ArraySlice : std::optional<UINT>{};
			std::optional<UINT> MipSlice   = RgUav.Desc.TextureUav.MipSlice != -1 ? RgUav.Desc.TextureUav.MipSlice : std::optional<UINT>{};
			UnorderedAccessViews[i]		   = D3D12UnorderedAccessView(
				   RenderCore::Device->GetDevice(),
				   &GetTexture(RgUav.Desc.Resource),
				   ArraySlice,
				   MipSlice);
		}
		break;

		default:
			assert(false && "Invalid Uav");
		}
	}
}

auto RenderGraphRegistry::GetBuffer(RgResourceHandle Handle) -> D3D12Buffer&
{
	assert(Handle.IsValid());
	assert(Handle.Type == RgResourceType::Buffer);
	assert(Handle.Id < Buffers.size());
	return Buffers[Handle.Id];
}

auto RenderGraphRegistry::GetTexture(RgResourceHandle Handle) -> D3D12Texture&
{
	assert(Handle.IsValid());
	assert(Handle.Type == RgResourceType::Texture);
	assert(Handle.Id < Textures.size());
	return Textures[Handle.Id];
}

auto RenderGraphRegistry::GetRenderTarget(RgResourceHandle Handle) -> D3D12RenderTarget&
{
	assert(Handle.IsValid());
	assert(Handle.Type == RgResourceType::RenderTarget);
	assert(Handle.Id < RenderTargets.size());
	return RenderTargets[Handle.Id];
}

auto RenderGraphRegistry::GetShaderResourceView(RgResourceHandle Handle) -> D3D12ShaderResourceView&
{
	assert(Handle.IsValid());
	assert(Handle.Type == RgResourceType::ShaderResourceView);
	assert(Handle.Id < ShaderResourceViews.size());
	return ShaderResourceViews[Handle.Id];
}

auto RenderGraphRegistry::GetUnorderedAccessView(RgResourceHandle Handle) -> D3D12UnorderedAccessView&
{
	assert(Handle.IsValid());
	assert(Handle.Type == RgResourceType::UnorderedAccessView);
	assert(Handle.Id < UnorderedAccessViews.size());
	return UnorderedAccessViews[Handle.Id];
}
