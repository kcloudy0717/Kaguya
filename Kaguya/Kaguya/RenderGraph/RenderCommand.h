#pragma once
#include "RenderGraphCommon.h"

// Still WIP
// The end goal here is to have a API agnostic high level command stream API and we "compile" the commands
// into API specific calls
// Refer to halcyon architecture

enum class ERenderCommandType
{
	Draw,
	DrawIndexed,
	Dispatch,

	NumRenderCommandTypes
};

template<ERenderCommandType Type>
struct TypedRenderCommand
{
	static constexpr ERenderCommandType Type = Type;
};

struct RenderCommandDraw : TypedRenderCommand<ERenderCommandType::Draw>
{
	RenderResourceHandle PipelineState;

	UINT VertexCountPerInstance;
	UINT InstanceCount;
	UINT StartVertexLocation;
	UINT StartInstanceLocation;
};

struct RenderCommandDrawIndexed : TypedRenderCommand<ERenderCommandType::DrawIndexed>
{
	RenderResourceHandle PipelineState;

	UINT IndexCountPerInstance;
	UINT InstanceCount;
	UINT StartIndexLocation;
	INT	 BaseVertexLocation;
	UINT StartInstanceLocation;
};

struct RenderCommandDispatch : TypedRenderCommand<ERenderCommandType::Dispatch>
{
	RenderResourceHandle PipelineState;

	UINT ThreadGroupCountX;
	UINT ThreadGroupCountY;
	UINT ThreadGroupCountZ;
};

struct RenderCommand
{
	ERenderCommandType Type;
	union
	{
		RenderCommandDraw		 Draw;
		RenderCommandDrawIndexed DrawIndexed;
		RenderCommandDispatch	 Dispatch;
	};
};

struct RenderCommandList
{
	void Draw(const RenderCommandDraw& Args)
	{
		RenderCommand& Command = Recorded.emplace_back();
		Command.Type		   = Args.Type;
		Command.Draw		   = Args;
	}

	void DrawIndexed(const RenderCommandDrawIndexed& Args)
	{
		RenderCommand& Command = Recorded.emplace_back();
		Command.Type		   = Args.Type;
		Command.DrawIndexed	   = Args;
	}

	void Dispatch(const RenderCommandDispatch& Args)
	{
		RenderCommand& Command = Recorded.emplace_back();
		Command.Type		   = Args.Type;
		Command.Dispatch	   = Args;
	}

	std::vector<RenderCommand> Recorded;
};
