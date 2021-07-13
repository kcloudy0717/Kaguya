#include "RenderDevice.h"

using Microsoft::WRL::ComPtr;

static RenderDevice* g_pRenderDevice = nullptr;

void RenderDevice::Initialize()
{
	if (!g_pRenderDevice)
	{
		g_pRenderDevice = new RenderDevice();
	}
}

void RenderDevice::Shutdown()
{
	if (g_pRenderDevice)
	{
		delete g_pRenderDevice;
	}
}

RenderDevice& RenderDevice::Instance()
{
	assert(RenderDevice::IsValid());
	return *g_pRenderDevice;
}

bool RenderDevice::IsValid()
{
	return g_pRenderDevice != nullptr;
}

const char* GetD3D12MessageSeverity(D3D12_MESSAGE_SEVERITY Severity)
{
	switch (Severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		return "[Corruption]";
	case D3D12_MESSAGE_SEVERITY_ERROR:
		return "[Error]";
	case D3D12_MESSAGE_SEVERITY_WARNING:
		return "[Warning]";
	case D3D12_MESSAGE_SEVERITY_INFO:
		return "[Info]";
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		return "[Message]";
	}

	return nullptr;
}

RenderDevice::RenderDevice()
{
	DeviceOptions  DeviceOptions  = { .EnableDebugLayer			= true,
									  .EnableGpuBasedValidation = true,
									  .EnableAutoDebugName		= true };
	DeviceFeatures DeviceFeatures = { .FeatureLevel = D3D_FEATURE_LEVEL_12_0, .WaveOperation = true };

	Adapter.Initialize(DeviceOptions);
	Adapter.InitializeDevice(DeviceFeatures);

	Adapter.RegisterMessageCallback(
		[](D3D12_MESSAGE_CATEGORY Category,
		   D3D12_MESSAGE_SEVERITY Severity,
		   D3D12_MESSAGE_ID		  ID,
		   LPCSTR				  pDescription,
		   void*				  pContext)
		{
			LOG_INFO("Severity: %s\n%s", GetD3D12MessageSeverity(Severity), pDescription);
		});

	m_SwapChain = SwapChain(
		Application::GetWindowHandle(),
		Adapter.GetFactory6(),
		Adapter.GetD3D12Device(),
		Adapter.GetDevice()->GetGraphicsQueue()->GetCommandQueue());

	UINT TempIndex = 0;
	Adapter.GetDevice()->GetResourceDescriptorHeap().Allocate(
		m_ImGuiFontCpuDescriptor,
		m_ImGuiFontGpuDescriptor,
		TempIndex);

	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(
		Adapter.GetD3D12Device(),
		1,
		SwapChain::Format,
		Adapter.GetDevice()->GetResourceDescriptorHeap(),
		m_ImGuiFontCpuDescriptor,
		m_ImGuiFontGpuDescriptor);
}

RenderDevice::~RenderDevice()
{
	ImGui_ImplDX12_Shutdown();
	Adapter.UnregisterMessageCallback();
}

void RenderDevice::Present(bool VSync)
{
	m_SwapChain.Present(VSync);
}

void RenderDevice::Resize(UINT Width, UINT Height)
{
	m_SwapChain.Resize(Width, Height);
}

RootSignature RenderDevice::CreateRootSignature(
	std::function<void(RootSignatureBuilder&)> Configurator,
	bool									   AddDescriptorTableRootParameters)
{
	RootSignatureBuilder builder = {};
	Configurator(builder);
	if (AddDescriptorTableRootParameters)
	{
		AddDescriptorTableRootParameterToBuilder(builder);
	}

	return RootSignature(Adapter.GetD3D12Device(), builder);
}

RaytracingPipelineState RenderDevice::CreateRaytracingPipelineState(
	std::function<void(RaytracingPipelineStateBuilder&)> Configurator)
{
	RaytracingPipelineStateBuilder builder = {};
	Configurator(builder);

	return RaytracingPipelineState(Adapter.GetD3D12Device5(), builder);
}

//
// void RenderDevice::CreateShaderResourceView(
//	ID3D12Resource*	  pResource,
//	const Descriptor& DestDescriptor,
//	UINT			  NumElements,
//	UINT			  Stride,
//	bool			  IsRawBuffer /*= false*/)
//{
//	const D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
//
//	switch (resourceDesc.Dimension)
//	{
//	case D3D12_RESOURCE_DIMENSION_BUFFER:
//		desc.Format						= DXGI_FORMAT_UNKNOWN;
//		desc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//		desc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
//		desc.Buffer.FirstElement		= 0;
//		desc.Buffer.NumElements			= NumElements;
//		desc.Buffer.StructureByteStride = Stride;
//		desc.Buffer.Flags				= IsRawBuffer ? D3D12_BUFFER_SRV_FLAG_NONE : D3D12_BUFFER_SRV_FLAG_RAW;
//		break;
//
//	default:
//		break;
//	}
//
//	m_Device->CreateShaderResourceView(pResource, &desc, DestDescriptor.CpuHandle);
//}
//

void RenderDevice::AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder)
{
	// TODO: Maybe consider this as a fallback options when SM6.6 dynamic resource binding is integrated
	/* Descriptor Tables */

	// ShaderResource
	DescriptorTable SRVTable;
	{
		// constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE |
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		SRVTable.AddSRVRange<0, 100>(UINT_MAX, Flags, 0); // g_Texture2DTable
		SRVTable.AddSRVRange<0, 101>(UINT_MAX, Flags, 0); // g_Texture2DUINT4Table
		SRVTable.AddSRVRange<0, 102>(UINT_MAX, Flags, 0); // g_Texture2DArrayTable
		SRVTable.AddSRVRange<0, 103>(UINT_MAX, Flags, 0); // g_TextureCubeTable
		SRVTable.AddSRVRange<0, 104>(UINT_MAX, Flags, 0); // g_ByteAddressBufferTable
	}
	RootSignatureBuilder.AddDescriptorTable(SRVTable);

	// UnorderedAccess
	DescriptorTable UAVTable;
	{
		// constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE |
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		UAVTable.AddUAVRange<0, 100>(UINT_MAX, Flags, 0); // g_RWTexture2DTable
		UAVTable.AddUAVRange<0, 101>(UINT_MAX, Flags, 0); // g_RWTexture2DArrayTable
		UAVTable.AddUAVRange<0, 102>(UINT_MAX, Flags, 0); // g_RWByteAddressBufferTable
	}
	RootSignatureBuilder.AddDescriptorTable(UAVTable);

	// Sampler
	DescriptorTable SamplerTable;
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		SamplerTable.AddSamplerRange<0, 100>(UINT_MAX, Flags, 0); // g_SamplerTable
	}
	RootSignatureBuilder.AddDescriptorTable(SamplerTable);
}
