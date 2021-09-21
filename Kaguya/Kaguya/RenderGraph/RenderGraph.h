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

	void AddRenderPass(RenderPass* RenderPass)
	{
		RenderPass->DependencyLevel = this;
		RenderPasses.push_back(RenderPass);
		Reads.insert(RenderPass->Reads.begin(), RenderPass->Reads.end());
		Writes.insert(RenderPass->Writes.begin(), RenderPass->Writes.end());
	}

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

	RenderGraph()
		: Scheduler(this)
		, Registry(Scheduler)
	{
	}

	auto begin() const noexcept { return RenderPasses.begin(); }
	auto end() const noexcept { return RenderPasses.end(); }

	void AddRenderPass(const std::string& Name, RenderPassCallback Callback)
	{
		RenderPass* NewRenderPass = new RenderPass(this, Name);
		Scheduler.SetCurrentRenderPass(NewRenderPass);
		NewRenderPass->Callback = Callback(Scheduler, NewRenderPass->Scope);
		Scheduler.SetCurrentRenderPass(nullptr);

		RenderPasses.emplace_back(NewRenderPass);
		LUT[Name] = NewRenderPass;
	}

	RenderPass* GetRenderPass(const std::string& Name) const
	{
		if (auto iter = LUT.find(Name); iter != LUT.end())
		{
			return iter->second;
		}
		return nullptr;
	}

	RenderScope& GetScope(const std::string& Name) const { return GetRenderPass(Name)->Scope; }

	RenderGraphScheduler& GetScheduler() { return Scheduler; }
	RenderGraphRegistry&  GetRegistry() { return Registry; }

	std::pair<UINT, UINT> GetRenderResolution() const { return { RenderWidth, RenderHeight }; }
	std::pair<UINT, UINT> GetViewportResolution() const { return { ViewportWidth, ViewportHeight }; }

	void Setup();
	void Compile();
	void Execute(D3D12CommandContext& Context);

	void SetResolution(UINT RenderWidth, UINT RenderHeight, UINT ViewportWidth, UINT ViewportHeight)
	{
		if (this->RenderWidth != RenderWidth || this->RenderHeight != RenderHeight)
		{
			this->RenderWidth		= RenderWidth;
			this->RenderHeight		= RenderHeight;
			RenderResolutionResized = true;
		}

		if (this->ViewportWidth != ViewportWidth || this->ViewportHeight != ViewportHeight)
		{
			this->ViewportWidth		  = ViewportWidth;
			this->ViewportHeight	  = ViewportHeight;
			ViewportResolutionResized = true;
		}
	}

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

public:
	bool ValidViewport = false;

private:
	std::vector<std::unique_ptr<RenderPass>>	 RenderPasses;
	std::unordered_map<std::string, RenderPass*> LUT;

	std::vector<std::vector<UINT64>> AdjacencyLists;
	std::vector<RenderPass*>		 TopologicalSortedPasses;

	std::vector<RenderGraphDependencyLevel> DependencyLevels;

	RenderGraphScheduler Scheduler;
	RenderGraphRegistry	 Registry;

	bool RenderResolutionResized = false;
	UINT RenderWidth, RenderHeight;
	bool ViewportResolutionResized = false;
	UINT ViewportWidth, ViewportHeight;
};
