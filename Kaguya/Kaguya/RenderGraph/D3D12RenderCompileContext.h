#pragma once
#include "RenderCompileContext.h"

class D3D12RenderCompileContext final : public RenderCompileContext
{
public:
	explicit D3D12RenderCompileContext(RenderDevice& Device, D3D12CommandContext& Context)
		: RenderCompileContext(Device)
		, Context(Context)
	{
	}

	bool CompileCommand(const RenderCommandSetCBV& Command) override;
	bool CompileCommand(const RenderCommandSetSRV& Command) override;
	bool CompileCommand(const RenderCommandSetUAV& Command) override;
	bool CompileCommand(const RenderCommandBeginRenderPass& Command) override;
	bool CompileCommand(const RenderCommandEndRenderPass& Command) override;
	bool CompileCommand(const RenderCommandPipelineState& Command) override;
	bool CompileCommand(const RenderCommandRaytracingPipelineState& Command) override;
	bool CompileCommand(const RenderCommandDraw& Command) override;
	bool CompileCommand(const RenderCommandDrawIndexed& Command) override;
	bool CompileCommand(const RenderCommandDispatch& Command) override;

private:
	D3D12CommandContext& Context;
};
