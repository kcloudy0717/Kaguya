#pragma once
#include "RenderDevice.h"

enum class EFSRQualityMode
{
	Ultra,
	Standard,
	Balanced,
	Performance
};

struct FSRState
{
	bool Enable = true;

	EFSRQualityMode QualityMode = EFSRQualityMode::Ultra;

	int ViewportWidth;
	int ViewportHeight;

	int	  RenderWidth;
	int	  RenderHeight;
	float RCASAttenuation = 0.0f;
};

class FSRFilter
{
public:
	FSRFilter() noexcept = default;

	void Initialize(RenderDevice& RenderDevice);

	void SetResolution(UINT Width, UINT Height);

	void Upscale(const FSRState& State, const ShaderResourceView& ShaderResourceView, CommandContext& Context);

	const ShaderResourceView& GetSRV() const { return SRVs[1]; }
	Texture*				  GetRenderTarget() { return &RenderTargets[1]; }

private:
	void ApplyEdgeAdaptiveSpatialUpsampling(
		UINT					  ThreadGroupCountX,
		UINT					  ThreadGroupCountY,
		const FSRState&			  State,
		const ShaderResourceView& ShaderResourceView,
		CommandContext&			  Context);
	void ApplyRobustContrastAdaptiveSharpening(
		UINT					  ThreadGroupCountX,
		UINT					  ThreadGroupCountY,
		const FSRState&			  State,
		const ShaderResourceView& ShaderResourceView,
		CommandContext&			  Context);

private:
	UINT Width = 0, Height = 0;

	RootSignature RS;
	PipelineState EASU_PSO;
	PipelineState RCAS_PSO;

	Texture				RenderTargets[2];
	UnorderedAccessView UAVs[2];
	ShaderResourceView	SRVs[2];
};
