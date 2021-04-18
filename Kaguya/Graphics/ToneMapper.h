#pragma once
#include "RenderDevice.h"

class ToneMapper
{
public:
	enum Operator
	{
		ACES
	};

	void Create();

	void SetResolution(UINT Width, UINT Height);

	void Apply(Descriptor ShaderResourceView, CommandList& CommandList);

	auto GetSRV() const
	{
		return m_SRV;
	}

	auto GetRenderTarget() const
	{
		return m_RenderTarget->pResource.Get();
	}
private:
	UINT m_Width = 0, m_Height = 0;

	RootSignature m_RS;
	PipelineState m_PSO;
	Descriptor m_RTV;
	Descriptor m_SRV;

	std::shared_ptr<Resource> m_RenderTarget;
};