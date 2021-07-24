#pragma once
#include "RenderDevice.h"

// TODO: Add more
enum class ETonemapOperator
{
	ACES
};

struct ToneMapperState
{
	ETonemapOperator TonemapOperator;
};

class ToneMapper
{
public:
	ToneMapper() noexcept = default;

	void Initialize(RenderDevice& RenderDevice);

	void SetResolution(UINT Width, UINT Height);

	void Apply(const ShaderResourceView& ShaderResourceView, CommandContext& Context);

	const ShaderResourceView& GetSRV() const { return SRV; }
	Texture*				  GetRenderTarget() { return &RenderTarget; }

private:
	UINT Width = 0, Height = 0;

	RootSignature RS;
	PipelineState PSO;

	Texture			   RenderTarget;
	RenderTargetView   RTV;
	ShaderResourceView SRV;
};
