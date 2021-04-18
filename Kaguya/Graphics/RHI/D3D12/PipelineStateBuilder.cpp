#include "pch.h"
#include "PipelineStateBuilder.h"

#include "Shader.h"

//----------------------------------------------------------------------------------------------------
D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateBuilder::Build()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = *pRootSignature;
	if (pVS) desc.VS = pVS->GetD3DShaderBytecode();
	if (pPS) desc.PS = pPS->GetD3DShaderBytecode();
	if (pDS) desc.DS = pDS->GetD3DShaderBytecode();
	if (pHS) desc.HS = pHS->GetD3DShaderBytecode();
	if (pGS) desc.GS = pGS->GetD3DShaderBytecode();
	desc.StreamOutput = {};
	desc.BlendState = BlendState;
	desc.SampleMask = DefaultSampleMask();
	desc.RasterizerState = RasterizerState;
	desc.DepthStencilState = DepthStencilState;
	desc.InputLayout = InputLayout;
	desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	desc.PrimitiveTopologyType = GetD3DPrimitiveTopologyType(PrimitiveTopology);
	desc.NumRenderTargets = NumRenderTargets;
	memcpy(desc.RTVFormats, RTVFormats, sizeof(desc.RTVFormats));
	desc.DSVFormat = DSVFormat;
	desc.SampleDesc = SampleDesc;
	desc.NodeMask = 0;
	desc.CachedPSO = {};
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	return desc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC GraphicsPipelineStateBuilder::BuildMSPSODesc()
{
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = *pRootSignature;
	desc.MS = pMS->GetD3DShaderBytecode();
	desc.PS = pPS->GetD3DShaderBytecode();
	desc.BlendState = BlendState;
	desc.SampleMask = DefaultSampleMask();
	desc.RasterizerState = RasterizerState;
	desc.DepthStencilState = DepthStencilState;
	desc.PrimitiveTopologyType = GetD3DPrimitiveTopologyType(PrimitiveTopology);
	desc.NumRenderTargets = NumRenderTargets;
	memcpy(desc.RTVFormats, RTVFormats, sizeof(desc.RTVFormats));
	desc.DSVFormat = DSVFormat;
	desc.SampleDesc = SampleDesc;
	desc.NodeMask = 0;
	desc.CachedPSO = {};
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	return desc;
}

void GraphicsPipelineStateBuilder::AddRenderTargetFormat(DXGI_FORMAT Format)
{
	assert(NumRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
	RTVFormats[NumRenderTargets++] = Format;
}

void GraphicsPipelineStateBuilder::SetDepthStencilFormat(DXGI_FORMAT Format)
{
	DSVFormat = Format;
}

void GraphicsPipelineStateBuilder::SetSampleDesc(UINT Count, UINT Quality)
{
	SampleDesc =
	{
		.Count = Count,
		.Quality = Quality
	};
}

//----------------------------------------------------------------------------------------------------
D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateBuilder::Build()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = *pRootSignature;
	desc.CS = pCS->GetD3DShaderBytecode();
	desc.NodeMask = 0;
	desc.CachedPSO = {};
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	return desc;
}