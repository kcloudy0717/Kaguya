#include "pch.h"
#include "ToneMapper.h"

#include "RendererRegistry.h"

void ToneMapper::Create()
{
	auto& RenderDevice = RenderDevice::Instance();

	m_RTV = RenderDevice.AllocateRenderTargetView();
	m_SRV = RenderDevice.AllocateShaderResourceView();

	m_RS = RenderDevice.CreateRootSignature([](RootSignatureBuilder& Builder)
	{
		Builder.AddRootConstantsParameter(RootConstants<void>(0, 0, 1)); // register(b0, space0)

		// PointClamp
		Builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);
	});

	m_PSO = RenderDevice.CreateGraphicsPipelineState([=](GraphicsPipelineStateBuilder& Builder)
	{
		Builder.pRootSignature = &m_RS;
		Builder.pVS = &Shaders::VS::FullScreenTriangle;
		Builder.pPS = &Shaders::PS::ToneMap;

		Builder.DepthStencilState.SetDepthEnable(false);

		Builder.PrimitiveTopology = PrimitiveTopology::Triangle;
		Builder.AddRenderTargetFormat(DirectX::MakeSRGB(RenderDevice::SwapChainBufferFormat));
	});
}

void ToneMapper::SetResolution(UINT Width, UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if (m_Width == Width && m_Height == Height)
	{
		return;
	}

	m_Width = Width;
	m_Height = Height;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(RenderDevice::SwapChainBufferFormat, Width, Height);
	resourceDesc.MipLevels = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	auto ClearValue = CD3DX12_CLEAR_VALUE(DirectX::MakeSRGB(RenderDevice::SwapChainBufferFormat), DirectX::Colors::White);

	m_RenderTarget = RenderDevice.CreateResource(&allocationDesc,
		&resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	RenderDevice.CreateRenderTargetView(m_RenderTarget->pResource.Get(), m_RTV, {}, {}, {}, true);
	RenderDevice.CreateShaderResourceView(m_RenderTarget->pResource.Get(), m_SRV);
}

void ToneMapper::Apply(Descriptor ShaderResourceView, CommandList& CommandList)
{
	PIXScopedEvent(CommandList.GetApiHandle(), 0, L"Tone Map");

	CommandList->SetPipelineState(m_PSO);
	CommandList->SetGraphicsRootSignature(m_RS);

	auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, m_Width, m_Height);
	auto scissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->RSSetViewports(1, &viewport);
	CommandList->RSSetScissorRects(1, &scissorRect);
	CommandList->OMSetRenderTargets(1, &m_RTV.CpuHandle, TRUE, nullptr);
	CommandList->ClearRenderTargetView(m_RTV.CpuHandle, DirectX::Colors::White, 0, nullptr);

	struct Settings
	{
		unsigned int InputIndex;
	} settings = {};
	settings.InputIndex = ShaderResourceView.Index;

	CommandList->SetGraphicsRoot32BitConstants(0, 1, &settings, 0);
	RenderDevice::Instance().BindDescriptorTable<PipelineState::Type::Graphics>(m_RS, CommandList);

	CommandList->DrawInstanced(3, 1, 0, 0);
}