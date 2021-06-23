#include "pch.h"
#include "ToneMapper.h"

#include "RendererRegistry.h"

ToneMapper::ToneMapper(_In_ RenderDevice& RenderDevice)
{
	m_RTV = RenderDevice.GetResourceViewHeaps().AllocateRenderTargetView();
	m_SRV = RenderDevice.GetResourceViewHeaps().AllocateResourceView();

	m_RS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 0>(1); // register(b0, space0)

			// PointClamp
			Builder.AddStaticSampler<0, 0>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);

			Builder.AllowResourceDescriptorHeapIndexing();
			Builder.AllowSampleDescriptorHeapIndexing();
		},
		false);

	auto depthStencilState						  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthStencilState.DepthEnable				  = FALSE;
	D3D12_RT_FORMAT_ARRAY formats				  = {};
	formats.RTFormats[formats.NumRenderTargets++] = SwapChain::Format_sRGB;

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE		pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY	PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS					VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS					PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL			DepthStencilState;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} stream;

	stream.pRootSignature		 = m_RS;
	stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stream.VS					 = Shaders::VS::FullScreenTriangle;
	stream.PS					 = Shaders::PS::ToneMap;
	stream.DepthStencilState	 = depthStencilState;
	stream.RTVFormats			 = formats;

	m_PSO = RenderDevice.CreatePipelineState(stream);
}

void ToneMapper::SetResolution(_In_ UINT Width, _In_ UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if (m_Width == Width && m_Height == Height)
	{
		return;
	}

	m_Width	 = Width;
	m_Height = Height;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags					= D3D12MA::ALLOCATION_FLAG_COMMITTED;
	allocationDesc.HeapType					= D3D12_HEAP_TYPE_DEFAULT;

	auto resourceDesc	   = CD3DX12_RESOURCE_DESC::Tex2D(SwapChain::Format, Width, Height);
	resourceDesc.MipLevels = 1;
	resourceDesc.Flags	   = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	FLOAT white[]	 = { 1, 1, 1, 1 };
	auto  ClearValue = CD3DX12_CLEAR_VALUE(SwapChain::Format_sRGB, white);

	m_RenderTarget =
		RenderDevice.CreateResource(&allocationDesc, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	RenderDevice.CreateRenderTargetView(m_RenderTarget->pResource.Get(), m_RTV, {}, {}, {}, true);
	RenderDevice.CreateShaderResourceView(m_RenderTarget->pResource.Get(), m_SRV);
}

void ToneMapper::Apply(_In_ Descriptor ShaderResourceView, _In_ CommandList& CommandList)
{
	PIXScopedEvent(CommandList.GetCommandList(), 0, L"Tonemapping");

	CommandList->SetPipelineState(m_PSO);
	CommandList->SetGraphicsRootSignature(m_RS);

	auto viewport	 = CD3DX12_VIEWPORT(0.0f, 0.0f, m_Width, m_Height);
	auto scissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->RSSetViewports(1, &viewport);
	CommandList->RSSetScissorRects(1, &scissorRect);
	CommandList->OMSetRenderTargets(1, &m_RTV.CpuHandle, TRUE, nullptr);

	FLOAT white[] = { 1, 1, 1, 1 };
	CommandList->ClearRenderTargetView(m_RTV.CpuHandle, white, 0, nullptr);

	struct Settings
	{
		unsigned int InputIndex;
	} settings			= {};
	settings.InputIndex = ShaderResourceView.Index;

	CommandList->SetGraphicsRoot32BitConstants(0, 1, &settings, 0);
	// RenderDevice::Instance().BindGraphicsDescriptorTable(m_RS, CommandList);

	CommandList->DrawInstanced(3, 1, 0, 0);
}
