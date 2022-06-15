#pragma once
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"

struct PostProcessInput
{
	RHI::RgResourceHandle Input;
	RHI::RgResourceHandle InputSrv;
};

struct PostProcessOutput
{
	RHI::RgResourceHandle Output;
	RHI::RgResourceHandle OutputSrv;
	RHI::RgResourceHandle OutputUav;
};

struct BlurUpsampleInputParameters
{
	RHI::RgResourceHandle  HighResInput; // [0]
	RHI::RgResourceHandle  LowResInput;
	RHI::RgResourceHandle* HighResOutput; // [1]

	RHI::RgResourceHandle HighResInputSrv;
	RHI::RgResourceHandle LowResInputSrv;
	RHI::RgResourceHandle HighResOutputUav;
};

class PostProcess
{
public:
	PostProcess(ShaderCompiler* Compiler, RHI::D3D12Device* Device);

	PostProcessOutput Apply(RHI::RenderGraph& Graph, View View, PostProcessInput Input);

private:
	void BlurUpsample(std::string_view Name, RHI::RenderGraph& Graph, BlurUpsampleInputParameters Inputs, float UpsampleInterpolationFactor);

private:
	Shader BloomMaskShader;
	Shader BloomDownsampleShader;
	Shader BloomBlurShader;
	Shader BloomUpsampleBlurShader;
	Shader TonemapShader;
	Shader BayerDitherShader;

	RHI::D3D12RootSignature PostProcessRS;

	RHI::D3D12PipelineState BloomMaskPSO;
	RHI::D3D12PipelineState BloomDownsamplePSO;
	RHI::D3D12PipelineState BloomBlurPSO;
	RHI::D3D12PipelineState BloomUpsampleBlurPSO;
	RHI::D3D12PipelineState TonemapPSO;
	RHI::D3D12PipelineState BayerDitherPSO;

public:
	bool  EnableBloom	 = false;
	float BloomThreshold = 3.5f;
	float BloomIntensity = 2.0f;

	bool EnableBayerDither = false;
};
