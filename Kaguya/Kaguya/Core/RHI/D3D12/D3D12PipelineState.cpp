#include "D3D12PipelineState.h"

D3D12PipelineState::D3D12PipelineState(ID3D12Device* Device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* Desc)
{
	VERIFY_D3D12_API(Device->CreateGraphicsPipelineState(Desc, IID_PPV_ARGS(&PipelineState)));
}

D3D12PipelineState::D3D12PipelineState(ID3D12Device* Device, const D3D12_COMPUTE_PIPELINE_STATE_DESC* Desc)
{
	VERIFY_D3D12_API(Device->CreateComputePipelineState(Desc, IID_PPV_ARGS(&PipelineState)));
}

D3D12PipelineState::D3D12PipelineState(ID3D12Device2* Device, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc)
{
	VERIFY_D3D12_API(Device->CreatePipelineState(&Desc, IID_PPV_ARGS(&PipelineState)));
}
