#include "pch.h"
#include "PipelineState.h"

PipelineState::PipelineState(ID3D12Device* pDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc)
{
	ASSERTD3D12APISUCCEEDED(pDevice->CreateGraphicsPipelineState(pDesc, IID_PPV_ARGS(&pPipelineState)));
}

PipelineState::PipelineState(ID3D12Device* pDevice, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc)
{
	ASSERTD3D12APISUCCEEDED(pDevice->CreateComputePipelineState(pDesc, IID_PPV_ARGS(&pPipelineState)));
}

PipelineState::PipelineState(ID3D12Device2* pDevice, const D3D12_PIPELINE_STATE_STREAM_DESC& Desc)
{
	ASSERTD3D12APISUCCEEDED(pDevice->CreatePipelineState(&Desc, IID_PPV_ARGS(&pPipelineState)));
}
