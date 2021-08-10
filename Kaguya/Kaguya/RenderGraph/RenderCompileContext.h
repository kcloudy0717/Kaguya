#pragma once
#include "RenderCommand.h"
#include "RenderDevice.h"

class RenderCompileContext
{
public:
	RenderCompileContext(RenderDevice& Device, D3D12CommandContext& Context)
		: Device(Device)
		, Context(Context)
	{
	}

	bool CompileCommand(const RenderCommandPipelineState& Command);
	bool CompileCommand(const RenderCommandRaytracingPipelineState& Command);
	bool CompileCommand(const RenderCommandDraw& Command);
	bool CompileCommand(const RenderCommandDrawIndexed& Command);
	bool CompileCommand(const RenderCommandDispatch& Command);

	void Translate(const RenderCommandList& CommandList);

private:
	RenderDevice&	Device;
	D3D12CommandContext& Context;
};
