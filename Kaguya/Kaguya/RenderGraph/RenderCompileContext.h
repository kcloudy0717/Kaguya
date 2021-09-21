#pragma once
#include "RenderCommand.h"
#include "RenderDevice.h"

class RenderCompileContext
{
public:
	explicit RenderCompileContext(RenderDevice& Device)
		: Device(Device)
	{
	}
	virtual ~RenderCompileContext() = default;

	virtual bool CompileCommand(const RenderCommandSetCBV& Command)					 = 0;
	virtual bool CompileCommand(const RenderCommandSetSRV& Command)					 = 0;
	virtual bool CompileCommand(const RenderCommandSetUAV& Command)					 = 0;
	virtual bool CompileCommand(const RenderCommandBeginRenderPass& Command)		 = 0;
	virtual bool CompileCommand(const RenderCommandEndRenderPass& Command)			 = 0;
	virtual bool CompileCommand(const RenderCommandPipelineState& Command)			 = 0;
	virtual bool CompileCommand(const RenderCommandRaytracingPipelineState& Command) = 0;
	virtual bool CompileCommand(const RenderCommandDraw& Command)					 = 0;
	virtual bool CompileCommand(const RenderCommandDrawIndexed& Command)			 = 0;
	virtual bool CompileCommand(const RenderCommandDispatch& Command)				 = 0;

	void Translate(const RenderCommandList& CommandList);

protected:
	RenderDevice& Device;
};
