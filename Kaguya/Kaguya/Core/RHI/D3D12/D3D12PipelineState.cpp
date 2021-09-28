#include "D3D12PipelineState.h"

D3D12PipelineState::D3D12PipelineState(D3D12Device* Parent, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* Desc)
	: D3D12DeviceChild(Parent)
	, Type(ED3D12PipelineStateType::Graphics)
{
	VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateGraphicsPipelineState(Desc, IID_PPV_ARGS(&PipelineState)));
}

D3D12PipelineState::D3D12PipelineState(D3D12Device* Parent, const D3D12_COMPUTE_PIPELINE_STATE_DESC* Desc)
	: D3D12DeviceChild(Parent)
	, Type(ED3D12PipelineStateType::Compute)
{
	VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateComputePipelineState(Desc, IID_PPV_ARGS(&PipelineState)));
}

D3D12PipelineState::D3D12PipelineState(D3D12Device* Parent, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc)
	: D3D12DeviceChild(Parent)
{
	VERIFY_D3D12_API(Parent->GetD3D12Device5()->CreatePipelineState(&Desc, IID_PPV_ARGS(&PipelineState)));
}
