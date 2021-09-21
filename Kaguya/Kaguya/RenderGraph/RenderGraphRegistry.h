#pragma once
#include "RenderGraphCommon.h"
#include "RenderGraphScheduler.h"

struct ShaderViews
{
	std::vector<D3D12ShaderResourceView>  SRVs;
	std::vector<D3D12UnorderedAccessView> UAVs;
};

class RenderGraphRegistry
{
public:
	RenderGraphRegistry(RenderGraphScheduler& Scheduler)
		: Scheduler(Scheduler)
	{
	}

	void Initialize();

	void RealizeResources();

	[[nodiscard]] auto GetTexture(RenderResourceHandle Handle) -> D3D12Texture&;

	[[nodiscard]] auto GetRenderTarget(RenderResourceHandle Handle) -> D3D12RenderTarget&;

	[[nodiscard]] auto GetTextureSRV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> const D3D12ShaderResourceView&;

	[[nodiscard]] auto GetTextureUAV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> const D3D12UnorderedAccessView&;

	[[nodiscard]] auto GetTextureIndex(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> UINT;

	[[nodiscard]] auto GetRWTextureIndex(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> UINT;

private:
	RenderGraphScheduler& Scheduler;

	std::vector<D3D12Texture> Textures;
	std::vector<ShaderViews>  TextureShaderViews;

	std::vector<D3D12RenderTarget> RenderTargets;
};
