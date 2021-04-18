#include "pch.h"
#include "PipelineState.h"
#include "PipelineStateBuilder.h"

PipelineState::PipelineState(ID3D12Device2* pDevice, GraphicsPipelineStateBuilder& Builder)
	: m_Type(PipelineState::Type::Graphics)
{
	pRootSignature = Builder.pRootSignature;

	if (!Builder.pMS)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = Builder.Build();
		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));
	}
	else
	{
		auto PSOStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(Builder.BuildMSPSODesc());

		D3D12_PIPELINE_STATE_STREAM_DESC Desc = {};
		Desc.pPipelineStateSubobjectStream = &PSOStream;
		Desc.SizeInBytes = sizeof(PSOStream);

		ThrowIfFailed(pDevice->CreatePipelineState(&Desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));
	}
}

PipelineState::PipelineState(ID3D12Device2* pDevice, ComputePipelineStateBuilder& Builder)
	: m_Type(PipelineState::Type::Compute)
{
	pRootSignature = Builder.pRootSignature;

	D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = Builder.Build();
	ThrowIfFailed(pDevice->CreateComputePipelineState(&Desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));
}