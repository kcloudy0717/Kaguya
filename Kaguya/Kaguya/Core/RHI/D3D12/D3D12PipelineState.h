#pragma once
#include "D3D12Common.h"

enum class ED3D12PipelineStateType
{
	Graphics,
	Compute,
	Mesh,
	Raytracing
};

class D3D12PipelineState : public D3D12DeviceChild
{
public:
	D3D12PipelineState() noexcept = default;
	D3D12PipelineState(D3D12Device* Parent, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* Desc);
	D3D12PipelineState(D3D12Device* Parent, const D3D12_COMPUTE_PIPELINE_STATE_DESC* Desc);
	D3D12PipelineState(D3D12Device* Parent, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc);
	template<typename PipelineStateStream>
	D3D12PipelineState(D3D12Device* Parent, PipelineStateStream& Stream)
		: D3D12PipelineState(
			  Parent,
			  D3D12_PIPELINE_STATE_STREAM_DESC{ .SizeInBytes				   = sizeof(PipelineStateStream),
												.pPipelineStateSubobjectStream = &Stream })
	{
	}

	operator ID3D12PipelineState*() const { return PipelineState.Get(); }

	[[nodiscard]] ID3D12PipelineState*	  GetApiHandle() const noexcept { return PipelineState.Get(); }
	[[nodiscard]] ED3D12PipelineStateType GetType() const noexcept { return Type; }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
	ED3D12PipelineStateType						Type;
};
