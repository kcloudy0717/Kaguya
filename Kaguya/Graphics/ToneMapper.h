#pragma once
#include "RenderDevice.h"

class ToneMapper
{
public:
	enum Operator
	{
		ACES
	};

	ToneMapper() noexcept = default;

	ToneMapper(RenderDevice& RenderDevice);

	ToneMapper(ToneMapper&&) noexcept = default;
	ToneMapper& operator=(ToneMapper&&) noexcept = default;

	void SetResolution(UINT Width, UINT Height);

	void Apply(const ShaderResourceView& ShaderResourceView, CommandContext& Context);

	const ShaderResourceView& GetSRV() const { return SRV; }
	Texture*				  GetRenderTarget() { return &RenderTarget; }

private:
	UINT m_Width = 0, m_Height = 0;

	RootSignature m_RS;
	PipelineState m_PSO;

	Texture			   RenderTarget;
	RenderTargetView   RTV;
	ShaderResourceView SRV;
};
