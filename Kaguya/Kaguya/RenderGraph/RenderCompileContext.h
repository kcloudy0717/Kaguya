#pragma once
#include "RenderCommand.h"
#include "RenderDevice.h"

class RenderCompileContext
{
public:
	RenderCompileContext(RenderDevice& Device, CommandContext& Context)
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
	CommandContext& Context;
};
