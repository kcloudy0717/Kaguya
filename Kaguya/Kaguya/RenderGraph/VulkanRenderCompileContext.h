#pragma once
#include "RenderCompileContext.h"

class VulkanRenderCompileContext final : public RenderCompileContext
{
public:
	explicit VulkanRenderCompileContext(RenderDevice& Device, VulkanCommandContext& Context)
		: RenderCompileContext(Device)
		, Context(Context)
	{
	}

	bool CompileCommand(const RenderCommandPipelineState& Command) override;
	bool CompileCommand(const RenderCommandRaytracingPipelineState& Command) override;
	bool CompileCommand(const RenderCommandDraw& Command) override;
	bool CompileCommand(const RenderCommandDrawIndexed& Command) override;
	bool CompileCommand(const RenderCommandDispatch& Command) override;

private:
	VulkanCommandContext& Context;
};
