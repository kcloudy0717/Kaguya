#pragma once
#include "RenderGraphCommon.h"

// Still WIP
// The end goal here is to have a API agnostic high level command stream API and we "compile" the commands
// into API specific calls
// Refer to halcyon architecture

enum class ERenderCommandType
{
	PipelineState,
	RaytracingPipelineState,

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

struct RenderCommandPipelineState : TypedRenderCommand<ERenderCommandType::PipelineState>
{
	RenderResourceHandle PipelineState;
};

struct RenderCommandRaytracingPipelineState : TypedRenderCommand<ERenderCommandType::RaytracingPipelineState>
{
	RenderResourceHandle RaytracingPipelineState;
};

struct RenderCommandDraw : TypedRenderCommand<ERenderCommandType::Draw>
{
	UINT VertexCountPerInstance;
	UINT InstanceCount;
	UINT StartVertexLocation;
	UINT StartInstanceLocation;
};

struct RenderCommandDrawIndexed : TypedRenderCommand<ERenderCommandType::DrawIndexed>
{
	UINT IndexCountPerInstance;
	UINT InstanceCount;
	UINT StartIndexLocation;
	INT	 BaseVertexLocation;
	UINT StartInstanceLocation;
};

struct RenderCommandDispatch : TypedRenderCommand<ERenderCommandType::Dispatch>
{
	UINT ThreadGroupCountX;
	UINT ThreadGroupCountY;
	UINT ThreadGroupCountZ;
};

struct RenderCommand
{
	ERenderCommandType Type;
	union
	{
		RenderCommandPipelineState			 PipelineState;
		RenderCommandRaytracingPipelineState RaytracingPipelineState;
		RenderCommandDraw					 Draw;
		RenderCommandDrawIndexed			 DrawIndexed;
		RenderCommandDispatch				 Dispatch;
	};
};

struct RenderCommandList
{
	void Reset() { Recorded.clear(); }

	void SetPipelineState(const RenderCommandPipelineState& Args)
	{
		RenderCommand& Command = Recorded.emplace_back();
		Command.Type		   = Args.Type;
		Command.PipelineState  = Args;
	}

	void SetRaytracingPipelineState(const RenderCommandRaytracingPipelineState& Args)
	{
		RenderCommand& Command			= Recorded.emplace_back();
		Command.Type					= Args.Type;
		Command.RaytracingPipelineState = Args;
	}

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
