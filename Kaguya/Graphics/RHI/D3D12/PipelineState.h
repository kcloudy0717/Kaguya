#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include "RootSignature.h"

class PipelineStateBuilder;
class GraphicsPipelineStateBuilder;
class ComputePipelineStateBuilder;

class PipelineState
{
public:
	enum class Type
	{
		Unknown,
		Graphics,
		Compute
	};

	PipelineState() = default;
	PipelineState(ID3D12Device2* pDevice, GraphicsPipelineStateBuilder& Builder);
	PipelineState(ID3D12Device2* pDevice, ComputePipelineStateBuilder& Builder);

	operator auto() const { return m_PipelineState.Get(); }

	inline auto GetType() const { return m_Type; }

	const RootSignature* pRootSignature;
protected:
	Type										m_Type				= Type::Unknown;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};