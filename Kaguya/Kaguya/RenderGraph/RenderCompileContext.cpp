#include "RenderCompileContext.h"

bool RenderCompileContext::CompileCommand(const RenderCommandPipelineState& Command)
{
	return true;
}

bool RenderCompileContext::CompileCommand(const RenderCommandRaytracingPipelineState& Command)
{
	return true;
}

bool RenderCompileContext::CompileCommand(const RenderCommandDraw& Command)
{
	return true;
}

bool RenderCompileContext::CompileCommand(const RenderCommandDrawIndexed& Command)
{
	return true;
}

bool RenderCompileContext::CompileCommand(const RenderCommandDispatch& Command)
{
	return true;
}

void RenderCompileContext::Translate(const RenderCommandList& CommandList)
{
#define COMPILE_PACKET(Type)                                                                                           \
	case ERenderCommandType::##Type:                                                                                   \
		CompileCommand(Command.##Type);                                                                                \
		break

	for (const auto& Command : CommandList.Recorded)
	{
		switch (Command.Type)
		{
			COMPILE_PACKET(PipelineState);
			COMPILE_PACKET(RaytracingPipelineState);
			COMPILE_PACKET(Draw);
			COMPILE_PACKET(DrawIndexed);
			COMPILE_PACKET(Dispatch);
		default:
			assert(false && "Failed");
			break;
		}
	}

#undef COMPILE_PACKET
}
