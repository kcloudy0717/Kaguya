#include "RenderGraph.h"
#include <iostream>

void RenderGraphDependencyLevel::AddRenderPass(RenderPass* RenderPass)
{
	RenderPass->DependencyLevel = this;
	RenderPasses.push_back(RenderPass);
	Reads.insert(RenderPass->Reads.begin(), RenderPass->Reads.end());
	Writes.insert(RenderPass->Writes.begin(), RenderPass->Writes.end());
}

void RenderGraphDependencyLevel::PostInitialize()
{
	for (auto Read : Reads)
	{
		D3D12_RESOURCE_STATES ReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		if (Parent->GetScheduler().AllowUnorderedAccess(Read))
		{
			ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}

		ReadStates.push_back(ReadState);
	}

	for (auto Write : Writes)
	{
		D3D12_RESOURCE_STATES WriteState = D3D12_RESOURCE_STATE_COMMON;

		if (Parent->GetScheduler().AllowRenderTarget(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		if (Parent->GetScheduler().AllowDepthStencil(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		if (Parent->GetScheduler().AllowUnorderedAccess(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		WriteStates.push_back(WriteState);
	}
}

void RenderGraphDependencyLevel::Execute(D3D12CommandContext& Context)
{
	for (auto Read : Reads)
	{
		D3D12_RESOURCE_STATES ReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		if (Parent->GetScheduler().AllowUnorderedAccess(Read))
		{
			ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}

		D3D12Texture& Texture = GetParentRenderGraph()->GetRegistry().GetTexture(Read);
		Context.TransitionBarrier(&Texture, ReadState);
	}
	for (auto Write : Writes)
	{
		D3D12_RESOURCE_STATES WriteState = D3D12_RESOURCE_STATE_COMMON;

		if (Parent->GetScheduler().AllowRenderTarget(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		}
		if (Parent->GetScheduler().AllowDepthStencil(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}
		if (Parent->GetScheduler().AllowUnorderedAccess(Write))
		{
			WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		D3D12Texture& Texture = GetParentRenderGraph()->GetRegistry().GetTexture(Write);
		Context.TransitionBarrier(&Texture, WriteState);
	}

	Context.FlushResourceBarriers();

	for (auto& RenderPass : RenderPasses)
	{
		RenderPass->Callback(GetParentRenderGraph()->GetRegistry(), Context);
	}
}

void RenderGraph::AddRenderPass(const std::string& Name, RenderPassCallback Callback)
{
	RenderPass* NewRenderPass = new RenderPass(this, Name);
	Scheduler.SetCurrentRenderPass(NewRenderPass);
	NewRenderPass->Callback = Callback(Scheduler, NewRenderPass->Scope);
	Scheduler.SetCurrentRenderPass(nullptr);

	RenderPasses.emplace_back(NewRenderPass);
	Lut[Name] = NewRenderPass;
}

RenderPass* RenderGraph::GetRenderPass(const std::string& Name) const
{
	if (auto iter = Lut.find(Name); iter != Lut.end())
	{
		return iter->second;
	}
	return nullptr;
}

RenderScope& RenderGraph::GetScope(const std::string& Name) const
{
	return GetRenderPass(Name)->Scope;
}

void RenderGraph::Setup()
{
	// https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
	// https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/
	// https://andrewcjp.wordpress.com/2019/09/28/the-render-graph-architecture/
	// https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in
	// https://media.contentapi.ea.com/content/dam/ea/seed/presentations/wihlidal-halcyonarchitecture-notes.pdf

	// Adjacency lists
	AdjacencyLists.resize(RenderPasses.size());

	for (size_t i = 0; i < RenderPasses.size(); ++i)
	{
		RenderPass* Node = RenderPasses[i].get();
		if (!Node->HasAnyDependencies())
		{
			continue;
		}

		std::vector<UINT64>& Indices = AdjacencyLists[i];

		for (size_t j = 0; j < RenderPasses.size(); ++j)
		{
			if (i == j)
			{
				continue;
			}

			RenderPass* Neighbor = RenderPasses[j].get();
			for (auto ReadResource : Neighbor->Reads)
			{
				// If other pass reads a subresource written by the current node, then it depends on current node and is
				// an adjacent dependency
				if (Node->Writes.find(ReadResource) != Node->Writes.end())
				{
					Indices.push_back(j);
					break;
				}
			}
		}
	}

	// Topological sort
	std::stack<size_t> Stack;
	std::vector<bool>  Visited(RenderPasses.size(), false);

	for (size_t i = 0; i < RenderPasses.size(); i++)
	{
		if (Visited[i] == false)
		{
			DepthFirstSearch(i, Visited, Stack);
		}
	}

	std::cout << "Topological Indices" << std::endl;
	while (!Stack.empty())
	{
		size_t i						  = Stack.top();
		RenderPasses[i]->TopologicalIndex = i;
		TopologicalSortedPasses.push_back(RenderPasses[i].get());
		// Debug
		std::cout << Stack.top() << " ";
		Stack.pop();
	}
	std::cout << std::endl;

	// Longest path search
	// Render passes in a dependency level share the same recursion depth,
	// or rather maximum recursion depth AKA longest path in a DAG
	std::vector<int> Distances(TopologicalSortedPasses.size(), 0);

	for (auto& TopologicalSortedPass : TopologicalSortedPasses)
	{
		size_t u = TopologicalSortedPass->TopologicalIndex;
		for (auto v : AdjacencyLists[u])
		{
			if (Distances[v] < Distances[u] + 1)
			{
				Distances[v] = Distances[u] + 1;
			}
		}
	}

	DependencyLevels.resize(
		*std::max_element(Distances.begin(), Distances.end()) + 1,
		RenderGraphDependencyLevel(this));
	for (size_t i = 0; i < TopologicalSortedPasses.size(); ++i)
	{
		int level = Distances[i];
		DependencyLevels[level].AddRenderPass(TopologicalSortedPasses[i]);
	}

	for (auto& DependencyLevel : DependencyLevels)
	{
		DependencyLevel.PostInitialize();
	}

	std::cout << "Longest Path" << std::endl;
	for (auto d : Distances)
	{
		std::cout << d << " ";
	}
	std::cout << std::endl;
}

void RenderGraph::Compile()
{
	Registry.Initialize();
}

void RenderGraph::Execute(D3D12CommandContext& Context)
{
	for (RHITexture& Texture : Scheduler.Textures)
	{
		if ((RenderResolutionResized && Texture.Desc.Resolution == ETextureResolution::Render) ||
			(ViewportResolutionResized && Texture.Desc.Resolution == ETextureResolution::Viewport))
		{
			Texture.Handle.State = ERGHandleState::Dirty;
		}
	}

	RenderResolutionResized = ViewportResolutionResized = false;

	Registry.RealizeResources();
	for (auto& RenderPass : RenderPasses)
	{
		auto& ViewData			= RenderPass->Scope.Get<RenderGraphViewData>();
		ViewData.RenderWidth	= RenderWidth;
		ViewData.RenderHeight	= RenderHeight;
		ViewData.ViewportWidth	= ViewportWidth;
		ViewData.ViewportHeight = ViewportHeight;
	}

	D3D12ScopedEvent(Context, "Render Graph");
	for (auto& DependencyLevel : DependencyLevels)
	{
		DependencyLevel.Execute(Context);
	}
}
