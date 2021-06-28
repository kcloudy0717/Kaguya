#pragma once

class PipelineState
{
public:
	PipelineState() noexcept = default;
	PipelineState(_In_ ID3D12Device* pDevice, _In_ const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc);
	PipelineState(_In_ ID3D12Device* pDevice, _In_ const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc);
	PipelineState(_In_ ID3D12Device2* pDevice, _In_ const D3D12_PIPELINE_STATE_STREAM_DESC& Desc);
	template<typename PipelineStateStream>
	PipelineState(_In_ ID3D12Device2* pDevice, _In_ PipelineStateStream& Stream)
		: PipelineState(
			  pDevice,
			  D3D12_PIPELINE_STATE_STREAM_DESC{ .SizeInBytes				   = sizeof(PipelineStateStream),
												.pPipelineStateSubobjectStream = &Stream })
	{
	}

	operator ID3D12PipelineState*() const { return pPipelineState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
};
