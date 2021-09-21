#include "RenderCompileContext.h"

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
			COMPILE_PACKET(SetCBV);
			COMPILE_PACKET(SetSRV);
			COMPILE_PACKET(SetUAV);
			COMPILE_PACKET(BeginRenderPass);
			COMPILE_PACKET(EndRenderPass);
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
