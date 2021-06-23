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
	ToneMapper(_In_ RenderDevice& RenderDevice);

	void SetResolution(_In_ UINT Width, _In_ UINT Height);

	void Apply(_In_ Descriptor ShaderResourceView, _In_ CommandList& CommandList);

	Descriptor		GetSRV() const { return m_SRV; }
	ID3D12Resource* GetRenderTarget() const { return m_RenderTarget->pResource.Get(); }

private:
	UINT m_Width = 0, m_Height = 0;

	RootSignature m_RS;
	PipelineState m_PSO;
	Descriptor	  m_RTV;
	Descriptor	  m_SRV;

	std::shared_ptr<Resource> m_RenderTarget;
};
