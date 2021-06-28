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

	void SetResolution(UINT Width, UINT Height);

	void Apply(const ShaderResourceView& ShaderResourceView, CommandContext& Context);

	const ShaderResourceView& GetSRV() const { return m_RenderTarget.SRV; }
	ID3D12Resource*			  GetRenderTarget() const { return m_RenderTarget.GetResource(); }

private:
	UINT m_Width = 0, m_Height = 0;

	RootSignature m_RS;
	PipelineState m_PSO;

	RenderTarget m_RenderTarget;
};
