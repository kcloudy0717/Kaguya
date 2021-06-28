#include "pch.h"
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
		Device::ReportLiveObjects();
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

RenderDevice::RenderDevice()
{
	InitializeDXGIObjects();

	DeviceOptions  DeviceOptions  = { .FeatureLevel				= D3D_FEATURE_LEVEL_12_0,
									  .EnableDebugLayer			= true,
									  .EnableGpuBasedValidation = false,
									  .BreakOnCorruption		= true,
									  .BreakOnError				= true,
									  .BreakOnWarning			= true,
									  .EnableAutoDebugName		= true };
	DeviceFeatures DeviceFeatures = { .WaveOperation = true };

	m_Device = std::make_unique<Device>(Adapter4.Get(), DeviceOptions);
	m_Device->Initialize(DeviceFeatures);

	m_SwapChain = SwapChain(
		Application::GetWindowHandle(),
		Factory6.Get(),
		m_Device->GetDevice(),
		m_Device->GetGraphicsQueue()->GetCommandQueue());

	UINT TempIndex = 0;
	m_Device->GetResourceViewHeaps().GetResourceDescriptorHeap().Allocate(
		m_ImGuiFontCpuDescriptor,
		m_ImGuiFontGpuDescriptor,
		TempIndex);

	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(
		m_Device->GetDevice(),
		1,
		SwapChain::Format_sRGB,
		m_Device->GetResourceViewHeaps().GetResourceDescriptorHeap(),
		m_ImGuiFontCpuDescriptor,
		m_ImGuiFontGpuDescriptor);
}

RenderDevice::~RenderDevice()
{
	ImGui_ImplDX12_Shutdown();
}

DXGI_QUERY_VIDEO_MEMORY_INFO RenderDevice::QueryLocalVideoMemoryInfo() const
{
	DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo = {};
	if (Adapter4)
	{
		Adapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemoryInfo);
	}
	return VideoMemoryInfo;
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

	return RootSignature(m_Device->GetDevice(), builder);
}

RaytracingPipelineState RenderDevice::CreateRaytracingPipelineState(
	std::function<void(RaytracingPipelineStateBuilder&)> Configurator)
{
	RaytracingPipelineStateBuilder builder = {};
	Configurator(builder);

	return RaytracingPipelineState(m_Device->GetDevice5(), builder);
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
// void RenderDevice::CreateShaderResourceView(
//	ID3D12Resource*		pResource,
//	const Descriptor&	DestDescriptor,
//	std::optional<UINT> MostDetailedMip /*= {}*/,
//	std::optional<UINT> MipLevels /*= {}*/)
//{
//	const D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
//
//	UINT mostDetailedMip = MostDetailedMip.value_or(0);
//	UINT mipLevels		 = MipLevels.value_or(resourceDesc.MipLevels);
//
//	auto GetValidSRVFormat = [](DXGI_FORMAT Format)
//	{
//		switch (Format)
//		{
//		case DXGI_FORMAT_R32_TYPELESS:
//			return DXGI_FORMAT_R32_FLOAT;
//		case DXGI_FORMAT_D24_UNORM_S8_UINT:
//			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
//		case DXGI_FORMAT_D32_FLOAT:
//			return DXGI_FORMAT_R32_FLOAT;
//		default:
//			return Format;
//		}
//	};
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
//	desc.Format							 = GetValidSRVFormat(resourceDesc.Format);
//	desc.Shader4ComponentMapping		 = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//
//	switch (resourceDesc.Dimension)
//	{
//	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//		if (resourceDesc.DepthOrArraySize > 1)
//		{
//			desc.ViewDimension						= D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
//			desc.Texture2DArray.MostDetailedMip		= mostDetailedMip;
//			desc.Texture2DArray.MipLevels			= mipLevels;
//			desc.Texture2DArray.ArraySize			= resourceDesc.DepthOrArraySize;
//			desc.Texture2DArray.PlaneSlice			= 0;
//			desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
//		}
//		else
//		{
//			desc.ViewDimension				   = D3D12_SRV_DIMENSION_TEXTURE2D;
//			desc.Texture2D.MostDetailedMip	   = mostDetailedMip;
//			desc.Texture2D.MipLevels		   = mipLevels;
//			desc.Texture2D.PlaneSlice		   = 0;
//			desc.Texture2D.ResourceMinLODClamp = 0.0f;
//		}
//		break;
//
//	default:
//		break;
//	}
//
//	m_Device->CreateShaderResourceView(pResource, &desc, DestDescriptor.CpuHandle);
//}
//
// void RenderDevice::CreateUnorderedAccessView(
//	ID3D12Resource*		pResource,
//	const Descriptor&	DestDescriptor,
//	std::optional<UINT> ArraySlice /*= {}*/,
//	std::optional<UINT> MipSlice /*= {}*/)
//{
//	const D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
//
//	UINT arraySlice = ArraySlice.value_or(0);
//	UINT mipSlice	= MipSlice.value_or(0);
//
//	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
//	desc.Format							  = resourceDesc.Format;
//
//	switch (resourceDesc.Dimension)
//	{
//	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//		if (resourceDesc.DepthOrArraySize > 1)
//		{
//			desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
//			desc.Texture2DArray.MipSlice		= mipSlice;
//			desc.Texture2DArray.FirstArraySlice = arraySlice;
//			desc.Texture2DArray.ArraySize		= resourceDesc.DepthOrArraySize;
//			desc.Texture2DArray.PlaneSlice		= 0;
//		}
//		else
//		{
//			desc.ViewDimension		  = D3D12_UAV_DIMENSION_TEXTURE2D;
//			desc.Texture2D.MipSlice	  = mipSlice;
//			desc.Texture2D.PlaneSlice = 0;
//		}
//		break;
//
//	default:
//		break;
//	}
//
//	m_Device->CreateUnorderedAccessView(pResource, nullptr, &desc, DestDescriptor.CpuHandle);
//}
//
// void RenderDevice::CreateRenderTargetView(
//	ID3D12Resource*		pResource,
//	const Descriptor&	DestDescriptor,
//	std::optional<UINT> ArraySlice /*= {}*/,
//	std::optional<UINT> MipSlice /*= {}*/,
//	std::optional<UINT> ArraySize /*= {}*/,
//	bool				sRGB /*= false*/)
//{
//	const D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
//
//	UINT arraySlice = ArraySlice.value_or(0);
//	UINT mipSlice	= MipSlice.value_or(0);
//	UINT arraySize	= ArraySize.value_or(resourceDesc.DepthOrArraySize);
//
//	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
//	desc.Format						   = sRGB ? DirectX::MakeSRGB(resourceDesc.Format) : resourceDesc.Format;
//
//	// TODO: Add 1D/3D support
//	switch (resourceDesc.Dimension)
//	{
//	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//		if (resourceDesc.DepthOrArraySize > 1)
//		{
//			desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
//			desc.Texture2DArray.MipSlice		= mipSlice;
//			desc.Texture2DArray.FirstArraySlice = arraySlice;
//			desc.Texture2DArray.ArraySize		= arraySize;
//			desc.Texture2DArray.PlaneSlice		= 0;
//		}
//		else
//		{
//			desc.ViewDimension		  = D3D12_RTV_DIMENSION_TEXTURE2D;
//			desc.Texture2D.MipSlice	  = mipSlice;
//			desc.Texture2D.PlaneSlice = 0;
//		}
//		break;
//
//	default:
//		break;
//	}
//
//	m_Device->CreateRenderTargetView(pResource, &desc, DestDescriptor.CpuHandle);
//}
//
// void RenderDevice::CreateDepthStencilView(
//	ID3D12Resource*		pResource,
//	const Descriptor&	DestDescriptor,
//	std::optional<UINT> ArraySlice /*= {}*/,
//	std::optional<UINT> MipSlice /*= {}*/,
//	std::optional<UINT> ArraySize /*= {}*/)
//{
//	const D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();
//
//	UINT arraySlice = ArraySlice.value_or(0);
//	UINT mipSlice	= MipSlice.value_or(0);
//	UINT arraySize	= ArraySize.value_or(resourceDesc.DepthOrArraySize);
//
//	auto GetValidDSVFormat = [](DXGI_FORMAT Format)
//	{
//		// TODO: Add more
//		switch (Format)
//		{
//		case DXGI_FORMAT_R32_TYPELESS:
//			return DXGI_FORMAT_D32_FLOAT;
//
//		default:
//			return DXGI_FORMAT_UNKNOWN;
//		}
//	};
//
//	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
//	desc.Format						   = GetValidDSVFormat(resourceDesc.Format);
//	desc.Flags						   = D3D12_DSV_FLAG_NONE;
//
//	switch (resourceDesc.Dimension)
//	{
//	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
//		if (resourceDesc.DepthOrArraySize > 1)
//		{
//			desc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
//			desc.Texture2DArray.MipSlice		= mipSlice;
//			desc.Texture2DArray.FirstArraySlice = arraySlice;
//			desc.Texture2DArray.ArraySize		= arraySize;
//		}
//		else
//		{
//			desc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
//			desc.Texture2D.MipSlice = mipSlice;
//		}
//		break;
//
//	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
//		throw std::invalid_argument("Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");
//
//	default:
//		break;
//	}
//
//	m_Device->CreateDepthStencilView(pResource, &desc, DestDescriptor.CpuHandle);
//}

void RenderDevice::InitializeDXGIObjects()
{
	constexpr bool DEBUG_MODE = _DEBUG;

	constexpr UINT flags = DEBUG_MODE ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	ThrowIfFailed(::CreateDXGIFactory2(flags, IID_PPV_ARGS(Factory6.ReleaseAndGetAddressOf())));

	// Enumerate hardware for an adapter that supports DX12
	ComPtr<IDXGIAdapter4> pAdapter4;
	UINT				  adapterID = 0;
	while (Factory6->EnumAdapterByGpuPreference(
			   adapterID,
			   DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			   IID_PPV_ARGS(pAdapter4.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC3 desc = {};
		ThrowIfFailed(pAdapter4->GetDesc3(&desc));

		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
		{
			// Skip SOFTWARE adapters
			continue;
		}

		if (SUCCEEDED(::D3D12CreateDevice(pAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			Adapter4	= pAdapter4;
			AdapterDesc = desc;
			break;
		}

		adapterID++;
	}
}

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
