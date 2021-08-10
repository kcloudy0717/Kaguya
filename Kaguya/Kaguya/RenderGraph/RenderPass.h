#pragma once
#include "RenderGraphCommon.h"

class D3D12CommandContext;

class RenderGraphRegistry;
class RenderGraphDependencyLevel;

class RenderPass : public RenderGraphChild
{
public:
	using ExecuteCallback = std::function<void(RenderGraphRegistry& Registry, D3D12CommandContext& Context)>;

	RenderPass(RenderGraph* Parent, const std::string& Name)
		: RenderGraphChild(Parent)
		, Name(Name)
	{
	}

	void Read(RenderResourceHandle Resource)
	{
		Reads.insert(Resource);
		ReadWrites.insert(Resource);
	}

	void Write(RenderResourceHandle Resource)
	{
		Writes.insert(Resource);
		ReadWrites.insert(Resource);
	}

	bool HasDependency(RenderResourceHandle Resource) const { return ReadWrites.find(Resource) != ReadWrites.end(); }

	bool HasAnyDependencies() const { return !ReadWrites.empty(); }

	std::string					Name;
	size_t						TopologicalIndex = 0;
	RenderGraphDependencyLevel* DependencyLevel	 = nullptr;

	std::unordered_set<RenderResourceHandle> Reads;
	std::unordered_set<RenderResourceHandle> Writes;
	std::unordered_set<RenderResourceHandle> ReadWrites;
	RenderScope								 Scope;

	ExecuteCallback Callback;
};
