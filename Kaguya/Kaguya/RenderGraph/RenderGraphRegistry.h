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

	void ScheduleResources();

	Texture& GetTexture(RenderResourceHandle Handle)
	{
		assert(Handle.Type == ERGResourceType::Texture);
		assert(Handle.Id >= 0 && Handle.Id < Textures.size());
		return Textures[Handle.Id];
	}

	const ShaderResourceView& GetTextureSRV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt);

	const UnorderedAccessView& GetTextureUAV(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt);

	UINT GetTextureIndex(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt);

	UINT GetRWTextureIndex(
		RenderResourceHandle Handle,
		std::optional<UINT>	 OptArraySlice = std::nullopt,
		std::optional<UINT>	 OptMipSlice   = std::nullopt,
		std::optional<UINT>	 OptPlaneSlice = std::nullopt);

private:
	RenderGraphScheduler& Scheduler;

	// std::vector<Buffer>	 Buffers;
	std::vector<Texture>	 Textures;
	std::vector<ShaderViews> TextureShaderViews;
};
