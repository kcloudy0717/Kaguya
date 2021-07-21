#include "FSRFilter.h"

#include "RendererRegistry.h"

// CAS
#define A_CPU
#include <Graphics/FidelityFX-FSR/ffx_a.h>
#include <Graphics/FidelityFX-FSR/ffx_fsr1.h>

struct RootConstants
{
	unsigned int InputTID;
	unsigned int OutputTID;
};

_declspec(align(256)) struct FSRConstants
{
	DirectX::XMUINT4 Const0;
	DirectX::XMUINT4 Const1;
	DirectX::XMUINT4 Const2;
	DirectX::XMUINT4 Const3;
	DirectX::XMUINT4 Sample;
};

FSRFilter::FSRFilter(RenderDevice& RenderDevice)
{
	FSR_RS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 0>(2);
			Builder.AddConstantBufferView<1, 0>();

			Builder.AddStaticSampler<0, 0>(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1);

			Builder.AllowResourceDescriptorHeapIndexing();
			Builder.AllowSampleDescriptorHeapIndexing();
		},
		false);

	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
		PSODesc.pRootSignature					  = FSR_RS;
		PSODesc.CS								  = Shaders::CS::EASU;
		EASU_PSO								  = RenderDevice.CreateComputePipelineState(PSODesc);
	}
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
		PSODesc.pRootSignature					  = FSR_RS;
		PSODesc.CS								  = Shaders::CS::RCAS;
		RCAS_PSO								  = RenderDevice.CreateComputePipelineState(PSODesc);
	}

	UAVs[0] = UnorderedAccessView(RenderDevice.GetDevice());
	UAVs[1] = UnorderedAccessView(RenderDevice.GetDevice());

	SRVs[0] = ShaderResourceView(RenderDevice.GetDevice());
	SRVs[1] = ShaderResourceView(RenderDevice.GetDevice());
}

void FSRFilter::SetResolution(UINT Width, UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if (this->Width == Width && this->Height == Height)
	{
		return;
	}

	this->Width	 = Width;
	this->Height = Height;

	auto TextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		SwapChain::Format,
		Width,
		Height,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	RenderTargets[0] = Texture(RenderDevice.GetDevice(), TextureDesc, {});
	RenderTargets[0].GetResource()->SetName(L"EASU Output");

	RenderTargets[0].CreateUnorderedAccessView(UAVs[0]);
	RenderTargets[0].CreateShaderResourceView(SRVs[0]);

	RenderTargets[1] = Texture(RenderDevice.GetDevice(), TextureDesc, {});
	RenderTargets[1].GetResource()->SetName(L"RCAS Output");

	RenderTargets[1].CreateUnorderedAccessView(UAVs[1]);
	RenderTargets[1].CreateShaderResourceView(SRVs[1]);
}

void FSRFilter::Upscale(const FSRState& State, const ShaderResourceView& ShaderResourceView, CommandContext& Context)
{
	D3D12ScopedEvent(Context, "FSR");

	Context->SetComputeRootSignature(FSR_RS);

	// This value is the image region dimension that each thread group of the FSR shader operates on
	constexpr UINT ThreadSize		 = 16;
	UINT		   ThreadGroupCountX = (State.ViewportWidth + (ThreadSize - 1)) / ThreadSize;
	UINT		   ThreadGroupCountY = (State.ViewportHeight + (ThreadSize - 1)) / ThreadSize;

	Context.TransitionBarrier(&RenderTargets[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	ApplyEdgeAdaptiveSpatialUpsampling(ThreadGroupCountX, ThreadGroupCountY, State, ShaderResourceView, Context);

	Context.TransitionBarrier(&RenderTargets[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	Context.TransitionBarrier(&RenderTargets[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	ApplyRobustContrastAdaptiveSharpening(ThreadGroupCountX, ThreadGroupCountY, State, SRVs[0], Context);

	Context.TransitionBarrier(&RenderTargets[1], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	Context.FlushResourceBarriers();
}

void FSRFilter::ApplyEdgeAdaptiveSpatialUpsampling(
	UINT					  ThreadGroupCountX,
	UINT					  ThreadGroupCountY,
	const FSRState&			  State,
	const ShaderResourceView& ShaderResourceView,
	CommandContext&			  Context)
{
	D3D12ScopedEvent(Context, "EASU");

	Context->SetPipelineState(EASU_PSO);

	RootConstants RC	  = { ShaderResourceView.GetIndex(), UAVs[0].GetIndex() };
	FSRConstants  FsrEasu = {};
	FsrEasuCon(
		reinterpret_cast<AU1*>(&FsrEasu.Const0),
		reinterpret_cast<AU1*>(&FsrEasu.Const1),
		reinterpret_cast<AU1*>(&FsrEasu.Const2),
		reinterpret_cast<AU1*>(&FsrEasu.Const3),
		static_cast<AF1>(State.RenderWidth),
		static_cast<AF1>(State.RenderHeight),
		static_cast<AF1>(State.RenderWidth),
		static_cast<AF1>(State.RenderHeight),
		(AF1)Width,
		(AF1)Height);
	FsrEasu.Sample.x = 0;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
	std::memcpy(Allocation.CPUVirtualAddress, &FsrEasu, sizeof(FSRConstants));

	Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
	Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);

	Context.Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
}

void FSRFilter::ApplyRobustContrastAdaptiveSharpening(
	UINT					  ThreadGroupCountX,
	UINT					  ThreadGroupCountY,
	const FSRState&			  State,
	const ShaderResourceView& ShaderResourceView,
	CommandContext&			  Context)
{
	D3D12ScopedEvent(Context, "RCAS");

	Context->SetPipelineState(RCAS_PSO);

	RootConstants RC	  = { ShaderResourceView.GetIndex(), UAVs[1].GetIndex() };
	FSRConstants  FsrRcas = {};
	FsrRcasCon(reinterpret_cast<AU1*>(&FsrRcas.Const0), State.RCASAttenuation);
	FsrRcas.Sample.x = 0;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
	std::memcpy(Allocation.CPUVirtualAddress, &FsrRcas, sizeof(FSRConstants));

	Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
	Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);

	Context.Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
}
