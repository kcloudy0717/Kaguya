#pragma once
#include "RenderGraphRegistry.h"
#include "DgmlBuilder.h"

#include <stack>

class RenderGraphAllocator
{
public:
	explicit RenderGraphAllocator(size_t SizeInBytes)
		: BaseAddress(std::make_unique<BYTE[]>(SizeInBytes))
		, Ptr(BaseAddress.get())
		, Sentinel(Ptr + SizeInBytes)
		, CurrentMemoryUsage(0)
	{
	}

	void* Allocate(size_t SizeInBytes, size_t Alignment)
	{
		SizeInBytes	 = AlignUp(SizeInBytes, Alignment);
		BYTE* Result = Ptr += SizeInBytes;
		assert(Result + SizeInBytes <= Sentinel);

		CurrentMemoryUsage += SizeInBytes;
		return Result;
	}

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

	void Reset()
	{
		Ptr				   = BaseAddress.get();
		CurrentMemoryUsage = 0;
	}

private:
	std::unique_ptr<BYTE[]> BaseAddress;
	BYTE*					Ptr;
	BYTE*					Sentinel;
	std::size_t				CurrentMemoryUsage;
};

class RenderPass
{
public:
	using ExecuteCallback = Delegate<void(RenderGraphRegistry& Registry, D3D12CommandContext& Context)>;

	RenderPass(std::string_view Name);

	RenderPass& Read(RgResourceHandle Resource);
	RenderPass& Write(RgResourceHandle* Resource);

	template<typename PFNRenderPassCallback>
	void Execute(PFNRenderPassCallback&& Callback)
	{
		this->Callback = std::move(Callback);
	}

	[[nodiscard]] bool HasDependency(RgResourceHandle Resource) const;
	[[nodiscard]] bool WritesTo(RgResourceHandle Resource) const;
	[[nodiscard]] bool ReadsFrom(RgResourceHandle Resource) const;

	[[nodiscard]] bool HasAnyDependencies() const noexcept;

	std::string_view Name;
	size_t			 TopologicalIndex = 0;

	std::unordered_set<RgResourceHandle> Reads;
	std::unordered_set<RgResourceHandle> Writes;
	std::unordered_set<RgResourceHandle> ReadWrites;

	ExecuteCallback Callback;
};

class RenderGraphDependencyLevel
{
public:
	void AddRenderPass(RenderPass* RenderPass);

	void Execute(RenderGraph* RenderGraph, D3D12CommandContext& Context);

private:
	std::vector<RenderPass*> RenderPasses;

	// Apply barriers at a dependency level to reduce redudant barriers
	std::unordered_set<RgResourceHandle> Reads;
	std::unordered_set<RgResourceHandle> Writes;
};

class RenderGraph
{
public:
	explicit RenderGraph(
		RenderGraphAllocator& Allocator,
		RenderGraphRegistry&  Registry);
	~RenderGraph();

	template<typename T>
	std::vector<typename RgResourceTraits<T>::Type>& GetContainer()
	{
		if constexpr (std::is_same_v<T, D3D12Buffer>)
		{
			return Buffers;
		}
		else if constexpr (std::is_same_v<T, D3D12Texture>)
		{
			return Textures;
		}
		else if constexpr (std::is_same_v<T, D3D12RenderTarget>)
		{
			return RenderTargets;
		}
		else if constexpr (std::is_same_v<T, D3D12ShaderResourceView>)
		{
			return ShaderResourceViews;
		}
		else if constexpr (std::is_same_v<T, D3D12UnorderedAccessView>)
		{
			return UnorderedAccessViews;
		}
	}

	template<typename T>
	auto Create(std::string_view Name, const typename RgResourceTraits<T>::Desc& Desc) -> RgResourceHandle
	{
		auto& Container = GetContainer<T>();

		RgResourceHandle Handle = {};
		Handle.Type				= RgResourceTraits<T>::Enum;
		Handle.State			= 1;
		Handle.Version			= 0;
		Handle.Id				= Container.size();

		auto& Resource	= Container.emplace_back();
		Resource.Name	= Name;
		Resource.Handle = Handle;
		Resource.Desc	= Desc;
		return Handle;
	}

	RenderPass& AddRenderPass(std::string_view Name);

	[[nodiscard]] RenderPass& GetEpiloguePass();

	[[nodiscard]] RenderGraphRegistry& GetRegistry();

	void Execute(D3D12CommandContext& Context);

	void ExportDgml(DgmlBuilder& Builder);

	[[nodiscard]] bool AllowRenderTarget(RgResourceHandle Resource) const noexcept;
	[[nodiscard]] bool AllowDepthStencil(RgResourceHandle Resource) const noexcept;
	[[nodiscard]] bool AllowUnorderedAccess(RgResourceHandle Resource) const noexcept;

private:
	void Setup();

	void DepthFirstSearch(size_t n, std::vector<bool>& Visited, std::stack<size_t>& Stack);

	std::string_view GetResourceName(RgResourceHandle Handle)
	{
		switch (Handle.Type)
		{
		case RgResourceType::Buffer:
			return Buffers[Handle.Id].Name;
		case RgResourceType::Texture:
			return Textures[Handle.Id].Name;
		}
		return "<unknown>";
	}

private:
	friend class RenderGraphRegistry;

	RenderGraphAllocator& Allocator;
	RenderGraphRegistry&  Registry;

	std::vector<RgBuffer>		Buffers;
	std::vector<RgTexture>		Textures;
	std::vector<RgRenderTarget> RenderTargets;
	std::vector<RgView>			ShaderResourceViews;
	std::vector<RgView>			UnorderedAccessViews;

	std::vector<RenderPass*> RenderPasses;
	RenderPass*				 EpiloguePass;

	std::vector<std::vector<UINT64>> AdjacencyLists;
	std::vector<RenderPass*>		 TopologicalSortedPasses;

	std::vector<RenderGraphDependencyLevel> DependencyLevels;
};
