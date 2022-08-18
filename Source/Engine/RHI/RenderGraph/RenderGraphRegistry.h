#pragma once
#include "RenderGraphCommon.h"

namespace RHI
{
	class RenderGraph;

	class RenderGraphRegistry
	{
	public:
		void RealizeResources(RenderGraph* Graph, D3D12Device* Device);

		D3D12Texture* GetImportedResource(RgResourceHandle Handle);

		template<typename T>
		[[nodiscard]] auto Get(RgResourceHandle Handle) -> T*
		{
			auto& Container = GetContainer<T>();
			assert(Handle.IsValid());
			assert(Handle.Type == RgResourceTraits<T>::Enum);
			assert(Handle.Id < Container.size());
			return &Container[Handle.Id];
		}

	private:
		template<typename T>
		[[nodiscard]] auto GetContainer() -> std::vector<typename RgResourceTraits<T>::ApiType>&
		{
			if constexpr (std::is_same_v<T, D3D12Buffer>)
			{
				return BufferCache;
			}
			else if constexpr (std::is_same_v<T, D3D12Texture>)
			{
				return TextureCache;
			}
			else if constexpr (std::is_same_v<T, D3D12RenderTargetView>)
			{
				return RtvCache;
			}
			else if constexpr (std::is_same_v<T, D3D12DepthStencilView>)
			{
				return DsvCache;
			}
			else if constexpr (std::is_same_v<T, D3D12ShaderResourceView>)
			{
				return SrvCache;
			}
			else if constexpr (std::is_same_v<T, D3D12UnorderedAccessView>)
			{
				return UavCache;
			}
		}

	private:
		bool IsViewDirty(const RgView& View);

	private:
		RenderGraph*					Graph = nullptr;

		// Cached desc table based on resource handle to prevent unnecessary recreation of resources
		std::unordered_map<RgResourceHandle, RgBufferDesc>	BufferDescTable;
		std::unordered_map<RgResourceHandle, RgTextureDesc> TextureDescTable;
		std::unordered_map<RgResourceHandle, RgViewDesc>	ViewDescTable;

		std::vector<D3D12Buffer>			  BufferCache;
		std::vector<D3D12Texture>			  TextureCache;
		std::vector<D3D12RenderTargetView>	  RtvCache;
		std::vector<D3D12DepthStencilView>	  DsvCache;
		std::vector<D3D12ShaderResourceView>  SrvCache;
		std::vector<D3D12UnorderedAccessView> UavCache;
	};
} // namespace RHI
