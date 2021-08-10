#include "PipelineState.h"

PipelineState::PipelineState(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc)
{
	VERIFY_D3D12_API(pDevice->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(&pPipelineState)));
}

PipelineState::PipelineState(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc)
{
	VERIFY_D3D12_API(pDevice->CreateComputePipelineState(pDesc, IID_PPV_ARGS(&pPipelineState)));
}

PipelineState::PipelineState(ID3D12Device2* pDevice, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc)
{
	VERIFY_D3D12_API(pDevice->CreatePipelineState(&Desc, IID_PPV_ARGS(&pPipelineState)));
}
