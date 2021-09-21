#pragma once
#include "RenderGraphCommon.h"

class D3D12CommandContext;

class RenderGraphRegistry;
class RenderGraphDependencyLevel;

class RenderPass : public RenderGraphChild
{
public:
	using ExecuteCallback = Delegate<void(RenderGraphRegistry& Registry, D3D12CommandContext& Context)>;

	RenderPass(RenderGraph* Parent, const std::string& Name);

	void Read(RenderResourceHandle Resource);
	void Write(RenderResourceHandle Resource);

	[[nodiscard]] bool HasDependency(RenderResourceHandle Resource) const;
	[[nodiscard]] bool HasWriteDependency(RenderResourceHandle Resource) const;
	[[nodiscard]] bool HasReadDependency(RenderResourceHandle Resource) const;

	[[nodiscard]] bool HasAnyDependencies() const noexcept;

	std::string					Name;
	size_t						TopologicalIndex = 0;
	RenderGraphDependencyLevel* DependencyLevel	 = nullptr;

	std::unordered_set<RenderResourceHandle> Reads;
	std::unordered_set<RenderResourceHandle> Writes;
	std::unordered_set<RenderResourceHandle> ReadWrites;
	RenderScope								 Scope;

	ExecuteCallback Callback;
};
