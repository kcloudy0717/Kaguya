#pragma once
#include <stack>

#include "System.h"
#include "RHI/RHI.h"
#include "RenderGraphRegistry.h"
#include "DgmlBuilder.h"

namespace RHI
{
	class RenderGraphAllocator
	{
	public:
		explicit RenderGraphAllocator(size_t SizeInBytes);

		void* Allocate(size_t SizeInBytes, size_t Alignment);

		template<typename T, typename... TArgs>
		T* Construct(TArgs&&... Args)
		{
			void* Memory = Allocate(sizeof(T), 16);
			return new (Memory) T(std::forward<TArgs>(Args)...);
		}

		template<typename T>
		static void Destruct(T* Ptr)
		{
			Ptr->~T();
		}

		void Reset();

	private:
		std::unique_ptr<std::byte[]> BaseAddress;
		std::byte*					 Ptr;
		std::byte*					 Sentinel;
		std::size_t					 CurrentMemoryUsage;
	};

	class RenderPass
	{
	public:
		using ExecuteCallback = Delegate<void(RenderGraphRegistry& Registry, D3D12CommandContext& Context)>;

		RenderPass(std::string_view Name);

		RenderPass& Read(Span<const RgResourceHandle> Resources);
		RenderPass& Write(Span<RgResourceHandle* const> Resources);

		template<typename PFNRenderPassCallback>
		void Execute(PFNRenderPassCallback&& Callback)
		{
			this->Callback = std::move(Callback);
		}

		[[nodiscard]] bool WritesTo(RgResourceHandle Resource) const;
		[[nodiscard]] bool ReadsFrom(RgResourceHandle Resource) const;

		[[nodiscard]] bool HasAnyDependencies() const noexcept;

		std::string_view Name;
		size_t			 TopologicalIndex = 0;

		robin_hood::unordered_set<RgResourceHandle> Reads;
		robin_hood::unordered_set<RgResourceHandle> Writes;
		robin_hood::unordered_set<RgResourceHandle> ReadWrites;

		ExecuteCallback Callback;
	};

	class RenderGraphLayer
	{
	public:
		void AddRenderPass(RenderPass* RenderPass);

		void Execute(RenderGraph* RenderGraph, D3D12CommandContext& Context);

	private:
		std::vector<RenderPass*> RenderPasses;

		// Apply barriers at a dependency level to reduce redudant barriers
		robin_hood::unordered_set<RgResourceHandle> Reads;
		robin_hood::unordered_set<RgResourceHandle> Writes;
		robin_hood::unordered_set<RgResourceHandle> ReadWrites;
	};

	class RenderGraph
	{
	public:
		explicit RenderGraph(
			RenderGraphAllocator& Allocator,
			RenderGraphRegistry&  Registry);
		~RenderGraph();

		// TODO: Add support for other rg resource types
		// Currently only support textures (mainly swapchain textures)
		auto Import(D3D12Texture* Texture) -> RgResourceHandle
		{
			RgResourceHandle Handle = {
				.Type	 = RgResourceType::Texture,
				.Flags	 = RG_RESOURCE_FLAG_IMPORTED,
				.Version = 0,
				.Id		 = ImportedTextures.size()

			};

			ImportedTextures.emplace_back(Texture);
			return Handle;
		}

		template<typename T>
		auto Create(const typename RgResourceTraits<T>::Desc& Desc) -> RgResourceHandle
		{
			auto& Container = GetContainer<T>();

			RgResourceHandle Handle = {
				.Type	 = RgResourceTraits<T>::Enum,
				.Flags	 = RG_RESOURCE_FLAG_NONE,
				.Version = 0,
				.Id		 = Container.size()
			};

			auto& Resource	= Container.emplace_back();
			Resource.Handle = Handle;
			Resource.Desc	= Desc;
			return Handle;
		}

		RenderPass& AddRenderPass(std::string_view Name);

		[[nodiscard]] RenderPass& GetProloguePass() const noexcept;
		[[nodiscard]] RenderPass& GetEpiloguePass() const noexcept;

		[[nodiscard]] RenderGraphRegistry& GetRegistry() const noexcept;

		void Execute(D3D12CommandContext& Context);

		void ExportDgml(DgmlBuilder& Builder) const;

		[[nodiscard]] bool AllowRenderTarget(RgResourceHandle Resource) const noexcept;
		[[nodiscard]] bool AllowDepthStencil(RgResourceHandle Resource) const noexcept;
		[[nodiscard]] bool AllowUnorderedAccess(RgResourceHandle Resource) const noexcept;

	private:
		void Setup();

		void DepthFirstSearch(size_t n, std::vector<bool>& Visited, std::stack<size_t>& Stack);

		[[nodiscard]] std::string_view GetResourceName(RgResourceHandle Handle) const
		{
			switch (Handle.Type)
			{
			case RgResourceType::Texture:
				return Textures[Handle.Id].Desc.Name;
			}
			return "<unknown>";
		}

		template<typename T>
		[[nodiscard]] auto GetContainer() -> std::vector<typename RgResourceTraits<T>::Type>&
		{
			if constexpr (std::is_same_v<T, D3D12Buffer>)
			{
				return Buffers;
			}
			else if constexpr (std::is_same_v<T, D3D12Texture>)
			{
				return Textures;
			}
			else if constexpr (std::is_same_v<T, D3D12RenderTargetView>)
			{
				return Rtvs;
			}
			else if constexpr (std::is_same_v<T, D3D12DepthStencilView>)
			{
				return Dsvs;
			}
			else if constexpr (std::is_same_v<T, D3D12ShaderResourceView>)
			{
				return Srvs;
			}
			else if constexpr (std::is_same_v<T, D3D12UnorderedAccessView>)
			{
				return Uavs;
			}
		}

	private:
		friend class RenderGraphRegistry;

		RenderGraphAllocator& Allocator;
		RenderGraphRegistry&  Registry;

		std::vector<D3D12Texture*> ImportedTextures;

		std::vector<RgBuffer>  Buffers;
		std::vector<RgTexture> Textures;
		std::vector<RgView>	   Rtvs;
		std::vector<RgView>	   Dsvs;
		std::vector<RgView>	   Srvs;
		std::vector<RgView>	   Uavs;

		std::vector<RenderPass*> RenderPasses;
		RenderPass*				 ProloguePass;
		RenderPass*				 EpiloguePass;

		std::vector<std::vector<u64>> AdjacencyLists;
		std::vector<RenderPass*>	  TopologicalSortedPasses;

		std::vector<RenderGraphLayer> Layers;
	};
} // namespace RHI
