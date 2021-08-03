#pragma once
#include "RenderGraphCommon.h"
#include "RenderGraphScheduler.h"

struct ShaderViews
{
	std::vector<ShaderResourceView>	 SRVs;
	std::vector<UnorderedAccessView> UAVs;
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

	[[nodiscard]] auto GetTexture(RenderResourceHandle Handle)->Texture&;

	[[nodiscard]] auto GetTextureSRV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> const ShaderResourceView&;

	[[nodiscard]] auto GetTextureUAV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt) -> const UnorderedAccessView&;

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

	// std::vector<Buffer>	 Buffers;
	std::vector<Texture>	 Textures;
	std::vector<ShaderViews> TextureShaderViews;
};
