#include "RenderPass.h"
#include "RenderGraph.h"

RenderPass::RenderPass(RenderGraph* Parent, const std::string& Name)
	: RenderGraphChild(Parent)
	, Name(Name)
{
}

void RenderPass::Read(RenderResourceHandle Resource)
{
	Reads.insert(Resource);
	ReadWrites.insert(Resource);
}

void RenderPass::Write(RenderResourceHandle Resource)
{
	Writes.insert(Resource);
	ReadWrites.insert(Resource);
}

bool RenderPass::HasDependency(RenderResourceHandle Resource) const
{
	return ReadWrites.find(Resource) != ReadWrites.end();
}

bool RenderPass::HasWriteDependency(RenderResourceHandle Resource) const
{
	return Writes.find(Resource) != Writes.end();
}

bool RenderPass::HasReadDependency(RenderResourceHandle Resource) const
{
	return Reads.find(Resource) != Reads.end();
}

bool RenderPass::HasAnyDependencies() const noexcept
{
	return !ReadWrites.empty();
}
