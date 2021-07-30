#pragma once
#include "RenderPass.h"
#include "RenderGraphScheduler.h"
#include "RenderGraphRegistry.h"
#include <stack>

class RenderGraphDependencyLevel
{
public:
	void AddRenderPass(const RenderPass* RenderPass)
	{
		RenderPasses.push_back(RenderPass);
		Reads.insert(RenderPass->Reads.begin(), RenderPass->Reads.end());
		Writes.insert(RenderPass->Writes.begin(), RenderPass->Writes.end());
	}

private:
	std::vector<const RenderPass*> RenderPasses;

	// Apply barriers at a dependency level to reduce redudant barriers
	std::unordered_set<RenderResourceHandle> Reads;
	std::unordered_set<RenderResourceHandle> Writes;
};

class RenderGraph
{
public:
	using RenderPassExecuteCallback = std::function<void(RenderGraphRegistry& Registry, CommandContext& Context)>;
	using RenderPassCallback =
		std::function<RenderPassExecuteCallback(RenderGraphScheduler& Scheduler, RenderScope& Scope)>;

	RenderGraph()
		: Scheduler(this)
		, Registry(Scheduler)
	{
	}

	void AddRenderPass(const std::string& Name, RenderPassCallback Callback)
	{
		RenderPass* NewRenderPass = new RenderPass(this, Name);
		Scheduler.SetCurrentRenderPass(NewRenderPass);
		Executables.push_back(Callback(Scheduler, NewRenderPass->Scope));

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

	RenderGraphRegistry& GetRegistry() { return Registry; }

	void Setup();
	void Compile();
	void Execute(CommandContext& Context);

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

private:
	std::vector<std::unique_ptr<RenderPass>>	 RenderPasses;
	std::unordered_map<std::string, RenderPass*> LUT;

	std::vector<std::vector<UINT64>> AdjacencyLists;
	std::vector<RenderPass*>		 TopologicalSortedPasses;

	std::vector<RenderPassExecuteCallback> Executables;

	std::vector<RenderGraphDependencyLevel> DependencyLevels;

	RenderGraphScheduler Scheduler;
	RenderGraphRegistry	 Registry;

	bool RenderResolutionResized = false;
	UINT RenderWidth, RenderHeight;
	bool ViewportResolutionResized = false;
	UINT ViewportWidth, ViewportHeight;
};
