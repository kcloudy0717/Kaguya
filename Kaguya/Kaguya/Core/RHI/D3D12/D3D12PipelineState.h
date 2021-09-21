#pragma once
#include "D3D12Common.h"

class D3D12PipelineState
{
public:
	D3D12PipelineState() noexcept = default;
	D3D12PipelineState(ID3D12Device* Device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* Desc);
	D3D12PipelineState(ID3D12Device* Device, const D3D12_COMPUTE_PIPELINE_STATE_DESC* Desc);
	D3D12PipelineState(ID3D12Device2* Device, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc);
	template<typename PipelineStateStream>
	D3D12PipelineState(ID3D12Device2* Device, PipelineStateStream& Stream)
		: D3D12PipelineState(
			  Device,
			  D3D12_PIPELINE_STATE_STREAM_DESC{ .SizeInBytes				   = sizeof(PipelineStateStream),
												.pPipelineStateSubobjectStream = &Stream })
	{
	}

	operator ID3D12PipelineState*() const { return PipelineState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};
