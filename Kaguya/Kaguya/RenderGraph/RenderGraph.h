#pragma once
#include "RenderPass.h"
#include "RenderGraphScheduler.h"
#include "RenderGraphRegistry.h"

#include "RenderDevice.h"
#include "RenderCompileContext.h"

#include <stack>

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

struct RenderGraphResolution
{
	bool RenderResolutionResized   = false;
	bool ViewportResolutionResized = false;
	UINT RenderWidth, RenderHeight;
	UINT ViewportWidth, ViewportHeight;
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
	}

	auto begin() const noexcept { return RenderPasses.begin(); }
	auto end() const noexcept { return RenderPasses.end(); }

	void AddRenderPass(const std::string& Name, RenderPassCallback Callback);

	[[nodiscard]] RenderPass* GetRenderPass(const std::string& Name) const;

	[[nodiscard]] RenderScope& GetScope(const std::string& Name) const;

	RenderGraphScheduler& GetScheduler() { return Scheduler; }
	RenderGraphRegistry&  GetRegistry() { return Registry; }

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
