#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include "PipelineState.h"
#include "../BlendState.h"
#include "../RasterizerState.h"
#include "../DepthStencilState.h"
#include "../InputLayout.h"

//----------------------------------------------------------------------------------------------------
class RootSignature;
class Shader;

//----------------------------------------------------------------------------------------------------
class PipelineStateBuilder
{
public:
	const RootSignature* pRootSignature = nullptr;
};

//----------------------------------------------------------------------------------------------------
class GraphicsPipelineStateBuilder : public PipelineStateBuilder
{
public:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC Build();
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC BuildMSPSODesc();

	void AddRenderTargetFormat(DXGI_FORMAT Format);
	void SetDepthStencilFormat(DXGI_FORMAT Format);
	void SetSampleDesc(UINT Count, UINT Quality);

	const Shader* pAS = nullptr;						// Default value: nullptr, optional set
	const Shader* pMS = nullptr;						// Default value: nullptr, optional set

	const Shader* pVS = nullptr;						// Default value: nullptr, optional set
	const Shader* pPS = nullptr;						// Default value: nullptr, optional set
	const Shader* pDS = nullptr;						// Default value: nullptr, optional set
	const Shader* pHS = nullptr;						// Default value: nullptr, optional set
	const Shader* pGS = nullptr;						// Default value: nullptr, optional set
	BlendState BlendState = {};							// Default value: Default, optional set
	RasterizerState RasterizerState = {};							// Default value: Default, optional set
	DepthStencilState DepthStencilState = {};							// Default value: Default, optional set
	InputLayout InputLayout = {};							// Default value: Default, optional set
	PrimitiveTopology PrimitiveTopology = PrimitiveTopology::Undefined;	// Default value: Undefined, must be set
	UINT NumRenderTargets = 0;							// Default value: 0, optional set
	DXGI_FORMAT RTVFormats[8] = {};							// Default value: DXGI_FORMAT_UNKNOWN, optional set
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;			// Default value: DXGI_FORMAT_UNKNOWN, optional set
	DXGI_SAMPLE_DESC SampleDesc = DefaultSampleDesc();			// Default value: Count: 1, Quality: 0
};

//----------------------------------------------------------------------------------------------------
class ComputePipelineStateBuilder : public PipelineStateBuilder
{
public:
	D3D12_COMPUTE_PIPELINE_STATE_DESC Build();

	const Shader* pCS = nullptr;	// Default value: nullptr, must be set
};