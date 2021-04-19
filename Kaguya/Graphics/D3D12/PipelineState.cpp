#include "pch.h"
#include "PipelineState.h"

PipelineState::PipelineState(
	_In_ ID3D12Device2* pDevice,
	_In_ const D3D12_PIPELINE_STATE_STREAM_DESC& Desc)
{
	ThrowIfFailed(pDevice->CreatePipelineState(&Desc, IID_PPV_ARGS(&m_PipelineState)));
}
