#pragma once

class PipelineState
{
public:
	PipelineState() noexcept = default;
	PipelineState(
		_In_ ID3D12Device2* pDevice,
		_In_ const D3D12_PIPELINE_STATE_STREAM_DESC& Desc);

	template<typename PipelineStateStream>
	PipelineState(
		_In_ ID3D12Device2* pDevice,
		_In_ PipelineStateStream& Stream)
		: PipelineState(pDevice,
			D3D12_PIPELINE_STATE_STREAM_DESC{
				.SizeInBytes = sizeof(PipelineStateStream),
				.pPipelineStateSubobjectStream = &Stream
			})
	{

	}

	PipelineState(PipelineState&&) noexcept = default;
	PipelineState& operator=(PipelineState&&) noexcept = default;

	PipelineState(const PipelineState&) = delete;
	PipelineState& operator=(const PipelineState&) = delete;

	operator ID3D12PipelineState* () const { return m_PipelineState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};
