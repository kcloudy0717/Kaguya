#include "ToneMapper.h"

#include "RendererRegistry.h"

void ToneMapper::Initialize(RenderDevice& RenderDevice)
{
	RTV = RenderTargetView(RenderDevice.GetDevice());
	SRV = ShaderResourceView(RenderDevice.GetDevice());

	RS = RenderDevice.CreateRootSignature(
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

	stream.pRootSignature		 = RS;
	stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stream.VS					 = Shaders::VS::FullScreenTriangle;
	stream.PS					 = Shaders::PS::ToneMap;
	stream.DepthStencilState	 = depthStencilState;
	stream.RTVFormats			 = formats;

	PSO = RenderDevice.CreatePipelineState(stream);
}

void ToneMapper::SetResolution(UINT Width, UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if (this->Width == Width && this->Height == Height)
	{
		return;
	}

	this->Width	 = Width;
	this->Height = Height;

	FLOAT Color[]	 = { 1, 1, 1, 1 };
	auto  ClearValue = CD3DX12_CLEAR_VALUE(SwapChain::Format_sRGB, Color);

	auto TextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		SwapChain::Format,
		Width,
		Height,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	RenderTarget = Texture(RenderDevice.GetDevice(), TextureDesc, ClearValue);
	RenderTarget.GetResource()->SetName(L"Tonemap Output");

	RenderTarget.CreateRenderTargetView(RTV, {}, {}, {}, true);
	RenderTarget.CreateShaderResourceView(SRV);
}

void ToneMapper::Apply(const ShaderResourceView& ShaderResourceView, CommandContext& Context)
{
	D3D12ScopedEvent(Context, "Tonemap");

	Context.TransitionBarrier(&RenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

	Context->SetPipelineState(PSO);
	Context->SetGraphicsRootSignature(RS);

	D3D12_VIEWPORT Viewport	   = CD3DX12_VIEWPORT(0.0f, 0.0f, Width, Height);
	D3D12_RECT	   ScissorRect = CD3DX12_RECT(0, 0, Width, Height);

	Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->RSSetViewports(1, &Viewport);
	Context->RSSetScissorRects(1, &ScissorRect);
	Context->OMSetRenderTargets(1, &RTV.GetCPUHandle(), TRUE, nullptr);

	FLOAT white[] = { 1, 1, 1, 1 };
	Context->ClearRenderTargetView(RTV.GetCPUHandle(), white, 0, nullptr);

	struct Settings
	{
		unsigned int InputIndex;
	} settings			= {};
	settings.InputIndex = ShaderResourceView.GetIndex();

	Context->SetGraphicsRoot32BitConstants(0, 1, &settings, 0);
	// RenderDevice::Instance().BindGraphicsDescriptorTable(m_RS, CommandList);

	Context.DrawInstanced(3, 1, 0, 0);

	Context.TransitionBarrier(&RenderTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	Context.FlushResourceBarriers();
}
