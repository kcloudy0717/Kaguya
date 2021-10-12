#pragma once
#include "RenderGraphScheduler.h"
#include "RenderGraphRegistry.h"

#include "RenderDevice.h"

#include <stack>

struct RenderGraphResolution
{
	void RefreshRenderResolution(UINT Width, UINT Height)
	{
		if (RenderWidth != Width || RenderHeight != Height)
		{
			RenderWidth				= Width;
			RenderHeight			= Height;
			RenderResolutionResized = true;
		}
	}
	void RefreshViewportResolution(UINT Width, UINT Height)
	{
		if (ViewportWidth != Width || ViewportHeight != Height)
		{
			ViewportWidth			  = Width;
			ViewportHeight			  = Height;
			ViewportResolutionResized = true;
		}
	}

	bool RenderResolutionResized   = false;
	bool ViewportResolutionResized = false;
	UINT RenderWidth, RenderHeight;
	UINT ViewportWidth, ViewportHeight;
};

class RenderGraphAllocator
{
public:
	explicit RenderGraphAllocator(size_t SizeInBytes)
		: BaseAddress(std::make_unique<BYTE[]>(SizeInBytes))
		, Ptr(BaseAddress.get())
		, Sentinel(Ptr + SizeInBytes)
	{
	}

	void* Allocate(size_t SizeInBytes, size_t Alignment)
	{
		SizeInBytes	 = AlignUp(SizeInBytes, Alignment);
		BYTE* Result = Ptr += SizeInBytes;
		assert(Result + SizeInBytes <= Sentinel);
		return Result;
	}

	template<typename T, typename... TArgs>
	T* Construct(TArgs&&... Args)
	{
		void* Memory = Allocate(sizeof(T), 16);
		return new (Memory) T(std::forward<TArgs>(Args)...);
	}

	template<typename T>
	void Destruct(T* Ptr)
	{
		Ptr->~T();
	}

	void Reset() { Ptr = BaseAddress.get(); }

private:
	std::unique_ptr<BYTE[]> BaseAddress;
	BYTE*					Ptr;
	BYTE*					Sentinel;
};

class RenderScope
{
public:
	// Every RenderScope will have RenderGraphViewData
	RenderScope() { Get<RenderGraphViewData>(); }

	template<typename T>
	T& Get()
	{
		static_assert(std::is_trivial_v<T>, "typename T is not Pod");

		auto& Data = DataTable[typeid(T)];
		if (!Data)
		{
			Data = std::make_unique<BYTE[]>(sizeof(T));
			new (Data.get()) T();
		}

		return *reinterpret_cast<T*>(Data.get());
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<BYTE[]>> DataTable;
};

class RenderPass : public RenderGraphChild
{
public:
	using ExecuteCallback = Delegate<void(RenderGraphRegistry& Registry, D3D12CommandContext& Context)>;

	RenderPass(RenderGraph* Parent, const std::string& Name);

	void Read(RenderResourceHandle Resource);
	void Write(RenderResourceHandle Resource);

	[[nodiscard]] bool HasDependency(RenderResourceHandle Resource) const;
	[[nodiscard]] bool WritesTo(RenderResourceHandle Resource) const;
	[[nodiscard]] bool ReadsFrom(RenderResourceHandle Resource) const;

	[[nodiscard]] bool HasAnyDependencies() const noexcept;

	std::string Name;
	size_t		TopologicalIndex = 0;

	std::unordered_set<RenderResourceHandle> Reads;
	std::unordered_set<RenderResourceHandle> Writes;
	std::unordered_set<RenderResourceHandle> ReadWrites;
	RenderScope								 Scope;

	ExecuteCallback Callback;
};

class RenderGraphDependencyLevel : public RenderGraphChild
{
public:
	RenderGraphDependencyLevel() noexcept = default;
	RenderGraphDependencyLevel(RenderGraph* Parent)
		: RenderGraphChild(Parent)
	{
	}

	void AddRenderPass(RenderPass* RenderPass);

	void PostInitialize();

	void Execute(D3D12CommandContext& Context);

private:
	std::vector<RenderPass*> RenderPasses;

	// Apply barriers at a dependency level to reduce redudant barriers
	std::unordered_set<RenderResourceHandle> Reads;
	std::unordered_set<RenderResourceHandle> Writes;
	std::vector<D3D12_RESOURCE_STATES>		 ReadStates;
	std::vector<D3D12_RESOURCE_STATES>		 WriteStates;
};

class RenderGraph
{
public:
	using RenderPassCallback =
		Delegate<RenderPass::ExecuteCallback(RenderGraphScheduler& Scheduler, RenderScope& Scope)>;

	explicit RenderGraph(
		RenderGraphAllocator&  Allocator,
		RenderGraphScheduler&  Scheduler,
		RenderGraphRegistry&   Registry,
		RenderGraphResolution& Resolution)
		: Allocator(Allocator)
		, Scheduler(Scheduler)
		, Registry(Registry)
		, Resolution(Resolution)
	{
		Allocator.Reset();
		Scheduler.Reset();
	}
	~RenderGraph()
	{
		for (auto RenderPass : RenderPasses)
		{
			Allocator.Destruct(RenderPass);
		}
		RenderPasses.clear();
		Lut.clear();
	}

	RenderPass* AddRenderPass(const std::string& Name, RenderPassCallback Callback);

	[[nodiscard]] RenderPass* GetRenderPass(const std::string& Name) const;

	[[nodiscard]] RenderScope& GetScope(const std::string& Name) const;

	[[nodiscard]] RenderGraphScheduler& GetScheduler() { return Scheduler; }
	[[nodiscard]] RenderGraphRegistry&	GetRegistry() { return Registry; }

	[[nodiscard]] std::pair<UINT, UINT> GetRenderResolution() const
	{
		return { Resolution.RenderWidth, Resolution.RenderHeight };
	}
	[[nodiscard]] std::pair<UINT, UINT> GetViewportResolution() const
	{
		return { Resolution.ViewportWidth, Resolution.ViewportHeight };
	}

	void Setup();
	void Compile();
	void Execute(D3D12CommandContext& Context);
	void RenderGui();

private:
	void DepthFirstSearch(size_t n, std::vector<bool>& Visited, std::stack<size_t>& Stack)
	{
		Visited[n] = true;

		for (auto i : AdjacencyLists[n])
		{
			if (!Visited[i])
			{
				DepthFirstSearch(i, Visited, Stack);
			}
		}

		Stack.push(n);
	}

private:
	RenderGraphAllocator&  Allocator;
	RenderGraphScheduler&  Scheduler;
	RenderGraphRegistry&   Registry;
	RenderGraphResolution& Resolution;

	bool GraphDirty = true;

	std::vector<RenderPass*>					 RenderPasses;
	std::unordered_map<std::string, RenderPass*> Lut;

	std::vector<std::vector<UINT64>> AdjacencyLists;
	std::vector<RenderPass*>		 TopologicalSortedPasses;

	std::vector<RenderGraphDependencyLevel> DependencyLevels;
};
