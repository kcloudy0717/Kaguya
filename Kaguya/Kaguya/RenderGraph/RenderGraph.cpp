#include "RenderGraph.h"
#include <iostream>

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
		RenderPass* CurrentRenderPass = RenderPasses[i].get();

		if (!CurrentRenderPass->HasAnyDependencies())
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

			RenderPass* pPassToCheck = RenderPasses[j].get();

			for (auto ReadResource : pPassToCheck->Reads)
			{
				// If other pass reads a subresource written by the current node, then it depends on current node and is
				// an adjacent dependency
				if (CurrentRenderPass->Writes.find(ReadResource) != CurrentRenderPass->Writes.end())
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

	for (size_t i = 0; i < TopologicalSortedPasses.size(); ++i)
	{
		size_t u = TopologicalSortedPasses[i]->TopologicalIndex;
		for (auto v : AdjacencyLists[u])
		{
			if (Distances[v] < Distances[u] + 1)
			{
				Distances[v] = Distances[u] + 1;
			}
		}
	}

	DependencyLevels.resize(*std::max_element(Distances.begin(), Distances.end()) + 1);
	for (size_t i = 0; i < TopologicalSortedPasses.size(); ++i)
	{
		int level = Distances[i];
		DependencyLevels[level].AddRenderPass(TopologicalSortedPasses[i]);
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

void RenderGraph::Execute(CommandContext& Context)
{
	for (RHITexture& Texture : Scheduler.Textures)
	{
		if ((RenderResolutionResized && Texture.Desc.TextureResolution == ETextureResolution::Render) ||
			(ViewportResolutionResized && Texture.Desc.TextureResolution == ETextureResolution::Viewport))
		{
			Texture.Handle.State = ERGHandleState::Dirty;
		}
	}

	RenderResolutionResized = ViewportResolutionResized = false;

	Registry.ScheduleResources();
	for (auto& RenderPass : RenderPasses)
	{
		auto& ViewData			= RenderPass->Scope.Get<RenderGraphViewData>();
		ViewData.RenderWidth	= RenderWidth;
		ViewData.RenderHeight	= RenderHeight;
		ViewData.ViewportWidth	= ViewportWidth;
		ViewData.ViewportHeight = ViewportHeight;
	}

	D3D12ScopedEvent(Context, "Render Graph");
	for (auto& Executable : Executables)
	{
		Executable(Registry, Context);
	}
}
