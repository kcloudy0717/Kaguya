#include "pch.h"
#include "RenderDevice.h"

using Microsoft::WRL::ComPtr;

static RenderDevice* g_pRenderDevice = nullptr;

namespace D3D12Utility
{
	void FlushCommandQueue(UINT64 Value, ID3D12Fence* pFence, ID3D12CommandQueue* pCommandQueue, wil::unique_event& Event)
	{
		ThrowIfFailed(pCommandQueue->Signal(pFence, Value));
		ThrowIfFailed(pFence->SetEventOnCompletion(Value, Event.get()));
		Event.wait();
	}
}

Resource::~Resource()
{
	SafeRelease(pAllocation);
}

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
		g_pRenderDevice->FlushGraphicsQueue();
		g_pRenderDevice->FlushComputeQueue();
		g_pRenderDevice->FlushCopyQueue();
		delete g_pRenderDevice;
	}
	Device::ReportLiveObjects();
}

RenderDevice& RenderDevice::Instance()
{
	assert(g_pRenderDevice);
	return *g_pRenderDevice;
}

RenderDevice::RenderDevice()
{
	InitializeDXGIObjects();

	Device.Create(m_DXGIAdapter.Get());

	GraphicsQueue = CommandQueue(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComputeQueue = CommandQueue(Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	CopyQueue = CommandQueue(Device, D3D12_COMMAND_LIST_TYPE_COPY);

	InitializeDXGISwapChain();

	m_GlobalOnlineDescriptorHeap = DescriptorHeap(Device, NumGlobalOnlineDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_GlobalOnlineSamplerDescriptorHeap = DescriptorHeap(Device, NumGlobalOnlineSamplerDescriptors, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_RenderTargetDescriptorHeap = DescriptorHeap(Device, NumRenderTargetDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DepthStencilDescriptorHeap = DescriptorHeap(Device, NumDepthStencilDescriptors, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	GraphicsFenceValue = ComputeFenceValue = CopyFenceValue = 1;

	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(GraphicsFence.ReleaseAndGetAddressOf())));
	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(ComputeFence.ReleaseAndGetAddressOf())));
	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(CopyFence.ReleaseAndGetAddressOf())));

	GraphicsEvent.create();
	ComputeEvent.create();
	CopyEvent.create();

	// Allocate RTV for SwapChain
	for (auto& swapChainDescriptor : m_SwapChainBufferDescriptors)
	{
		swapChainDescriptor = AllocateRenderTargetView();
	}

	m_ImGuiDescriptor = AllocateShaderResourceView();
	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(Device, 1,
		RenderDevice::SwapChainBufferFormat, m_GlobalOnlineDescriptorHeap,
		m_ImGuiDescriptor.CpuHandle,
		m_ImGuiDescriptor.GpuHandle);
}

RenderDevice::~RenderDevice()
{
	ImGui_ImplDX12_Shutdown();
}

void RenderDevice::CreateCommandContexts(UINT NumGraphicsContext)
{
	// + 1 for Default
	NumGraphicsContext = NumGraphicsContext + 1;
	constexpr UINT NumAsyncComputeContext = 1;
	constexpr UINT NumCopyContext = 1;

	m_GraphicsContexts.reserve(NumGraphicsContext);
	m_AsyncComputeContexts.reserve(NumAsyncComputeContext);
	m_CopyContexts.reserve(NumCopyContext);

	for (UINT i = 0; i < NumGraphicsContext; ++i)
	{
		m_GraphicsContexts.emplace_back(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	for (UINT i = 0; i < NumAsyncComputeContext; ++i)
	{
		m_AsyncComputeContexts.emplace_back(Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	}

	for (UINT i = 0; i < NumCopyContext; ++i)
	{
		m_CopyContexts.emplace_back(Device, D3D12_COMMAND_LIST_TYPE_COPY);
	}
}

DXGI_QUERY_VIDEO_MEMORY_INFO RenderDevice::QueryLocalVideoMemoryInfo() const
{
	DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo = {};
	if (m_DXGIAdapter)
	{
		m_DXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo);
	}
	return memoryInfo;
}

void RenderDevice::Present(bool VSync)
{
	UINT syncInterval = VSync ? 1u : 0u;
	UINT presentFlags = (m_TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	HRESULT hr = m_DXGISwapChain->Present(syncInterval, presentFlags);
	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		// TODO: Handle device removal
		LOG_ERROR("DXGI_ERROR_DEVICE_REMOVED");
	}

	m_BackBufferIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
}

void RenderDevice::Resize(UINT Width, UINT Height)
{
	// Release resources before resize swap chain
	for (auto& swapChainBuffer : m_SwapChainBuffers)
	{
		swapChainBuffer.Reset();
	}

	// Resize backbuffer
	// Note: Cannot use ResizeBuffers1 when debugging in Nsight Graphics, it will crash
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	ThrowIfFailed(m_DXGISwapChain->GetDesc1(&desc));
	ThrowIfFailed(m_DXGISwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, desc.Flags));

	// Recreate descriptors
	for (uint32_t i = 0; i < RenderDevice::NumSwapChainBuffers; ++i)
	{
		ComPtr<ID3D12Resource> pBackBuffer;
		ThrowIfFailed(m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(pBackBuffer.ReleaseAndGetAddressOf())));

		CreateRenderTargetView(pBackBuffer.Get(), m_SwapChainBufferDescriptors[i]);

		m_SwapChainBuffers[i] = std::move(pBackBuffer);
	}

	ScopedWriteLock SWL(m_GlobalResourceStateTrackerLock);
	for (auto& swapChainBuffer : m_SwapChainBuffers)
	{
		m_GlobalResourceStateTracker.AddResourceState(swapChainBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
	}

	// Reset back buffer index
	m_BackBufferIndex = 0;
}

void RenderDevice::BindGlobalDescriptorHeap(CommandList& CommandList)
{
	CommandList.SetDescriptorHeaps(&m_GlobalOnlineDescriptorHeap, &m_GlobalOnlineSamplerDescriptorHeap);
}

void RenderDevice::FlushGraphicsQueue()
{
	UINT64 value = ++GraphicsFenceValue;
	D3D12Utility::FlushCommandQueue(value, GraphicsFence.Get(), GraphicsQueue, GraphicsEvent);
}

void RenderDevice::FlushComputeQueue()
{
	UINT64 value = ++ComputeFenceValue;
	D3D12Utility::FlushCommandQueue(value, ComputeFence.Get(), ComputeQueue, ComputeEvent);
}

void RenderDevice::FlushCopyQueue()
{
	UINT64 value = ++CopyFenceValue;
	D3D12Utility::FlushCommandQueue(value, CopyFence.Get(), CopyQueue, CopyEvent);
}

std::shared_ptr<Resource> RenderDevice::CreateResource(const D3D12MA::ALLOCATION_DESC* pAllocDesc,
	const D3D12_RESOURCE_DESC* pResourceDesc,
	D3D12_RESOURCE_STATES InitialResourceState,
	const D3D12_CLEAR_VALUE* pOptimizedClearValue)
{
	std::shared_ptr<Resource> pResource = std::make_shared<Resource>();

	ThrowIfFailed(Device.Allocator()->CreateResource(pAllocDesc,
		pResourceDesc, InitialResourceState, pOptimizedClearValue,
		&pResource->pAllocation, IID_PPV_ARGS(pResource->pResource.ReleaseAndGetAddressOf())));

	// No need to track resources that have constant resource state throughout their lifetime
	if (pAllocDesc->HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		ScopedWriteLock _(m_GlobalResourceStateTrackerLock);
		m_GlobalResourceStateTracker.AddResourceState(pResource->pResource.Get(), InitialResourceState);
	}

	return pResource;
}

RootSignature RenderDevice::CreateRootSignature(std::function<void(RootSignatureBuilder&)> Configurator, bool AddDescriptorTableRootParameters)
{
	RootSignatureBuilder builder = {};
	Configurator(builder);
	if (AddDescriptorTableRootParameters)
	{
		AddDescriptorTableRootParameterToBuilder(builder);
	}

	return RootSignature(Device, builder);
}

PipelineState RenderDevice::CreateGraphicsPipelineState(std::function<void(GraphicsPipelineStateBuilder&)> Configurator)
{
	GraphicsPipelineStateBuilder builder = {};
	Configurator(builder);

	return PipelineState(Device, builder);
}

PipelineState RenderDevice::CreateComputePipelineState(std::function<void(ComputePipelineStateBuilder&)> Configurator)
{
	ComputePipelineStateBuilder builder = {};
	Configurator(builder);

	return PipelineState(Device, builder);
}

RaytracingPipelineState RenderDevice::CreateRaytracingPipelineState(std::function<void(RaytracingPipelineStateBuilder&)> Configurator)
{
	RaytracingPipelineStateBuilder builder = {};
	Configurator(builder);

	return RaytracingPipelineState(Device, builder);
}

Descriptor RenderDevice::AllocateShaderResourceView()
{
	UINT index = m_GlobalOnlineDescriptorIndexPool.allocate();
	return m_GlobalOnlineDescriptorHeap.At(index);
}

Descriptor RenderDevice::AllocateUnorderedAccessView()
{
	UINT index = m_GlobalOnlineDescriptorIndexPool.allocate();
	return m_GlobalOnlineDescriptorHeap.At(index);
}

Descriptor RenderDevice::AllocateRenderTargetView()
{
	UINT index = m_RenderTargetDescriptorIndexPool.allocate();
	return m_RenderTargetDescriptorHeap.At(index);
}

Descriptor RenderDevice::AllocateDepthStencilView()
{
	UINT index = m_DepthStencilDescriptorIndexPool.allocate();
	return m_DepthStencilDescriptorHeap.At(index);
}

void RenderDevice::ReleaseShaderResourceView(Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_GlobalOnlineDescriptorIndexPool.free(Descriptor.Index);
	}
}

void RenderDevice::ReleaseUnorderedAccessView(Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_GlobalOnlineDescriptorIndexPool.free(Descriptor.Index);
	}
}

void RenderDevice::ReleaseRenderTargetView(Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_RenderTargetDescriptorIndexPool.free(Descriptor.Index);
	}
}

void RenderDevice::ReleaseDepthStencilView(Descriptor Descriptor)
{
	if (Descriptor.IsValid())
	{
		m_DepthStencilDescriptorIndexPool.free(Descriptor.Index);
	}
}

void RenderDevice::CreateShaderResourceView(ID3D12Resource* pResource,
	const Descriptor& DestDescriptor,
	UINT NumElements,
	UINT Stride,
	bool IsRawBuffer /*= false*/)
{
	const auto resourceDesc = pResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};

	switch (resourceDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_BUFFER:
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = NumElements;
		desc.Buffer.StructureByteStride = Stride;
		desc.Buffer.Flags = IsRawBuffer ? D3D12_BUFFER_SRV_FLAG_NONE : D3D12_BUFFER_SRV_FLAG_RAW;
		break;

	default:
		break;
	}

	Device->CreateShaderResourceView(pResource, &desc, DestDescriptor.CpuHandle);
}

void RenderDevice::CreateShaderResourceView(ID3D12Resource* pResource,
	const Descriptor& DestDescriptor,
	std::optional<UINT> MostDetailedMip /*= {}*/,
	std::optional<UINT> MipLevels /*= {}*/)
{
	const auto resourceDesc = pResource->GetDesc();

	UINT mostDetailedMip = MostDetailedMip.value_or(0);
	UINT mipLevels = MipLevels.value_or(resourceDesc.MipLevels);

	auto GetValidSRVFormat = [](DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R32_TYPELESS:		return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT:			return DXGI_FORMAT_R32_FLOAT;
		default:							return Format;
		}
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = GetValidSRVFormat(resourceDesc.Format);
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (resourceDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resourceDesc.DepthOrArraySize > 1)
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MostDetailedMip = mostDetailedMip;
			desc.Texture2DArray.MipLevels = mipLevels;
			desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
			desc.Texture2DArray.PlaneSlice = 0;
			desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = mostDetailedMip;
			desc.Texture2D.MipLevels = mipLevels;
			desc.Texture2D.PlaneSlice = 0;
			desc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		break;

	default:
		break;
	}

	Device->CreateShaderResourceView(pResource, &desc, DestDescriptor.CpuHandle);
}

void RenderDevice::CreateUnorderedAccessView(ID3D12Resource* pResource,
	const Descriptor& DestDescriptor,
	std::optional<UINT> ArraySlice /*= {}*/,
	std::optional<UINT> MipSlice /*= {}*/)
{
	const auto resourceDesc = pResource->GetDesc();

	UINT arraySlice = ArraySlice.value_or(0);
	UINT mipSlice = MipSlice.value_or(0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.Format = resourceDesc.Format;

	switch (resourceDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resourceDesc.DepthOrArraySize > 1)
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mipSlice;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
			desc.Texture2DArray.PlaneSlice = 0;
		}
		else
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
			desc.Texture2D.PlaneSlice = 0;
		}
		break;

	default:
		break;
	}

	Device->CreateUnorderedAccessView(pResource, nullptr, &desc, DestDescriptor.CpuHandle);
}

void RenderDevice::CreateRenderTargetView(ID3D12Resource* pResource,
	const Descriptor& DestDescriptor,
	std::optional<UINT> ArraySlice /*= {}*/,
	std::optional<UINT> MipSlice /*= {}*/,
	std::optional<UINT> ArraySize /*= {}*/,
	bool sRGB /*= false*/)
{
	const auto resourceDesc = pResource->GetDesc();

	UINT arraySlice = ArraySlice.value_or(0);
	UINT mipSlice = MipSlice.value_or(0);
	UINT arraySize = ArraySize.value_or(resourceDesc.DepthOrArraySize);

	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = sRGB ? DirectX::MakeSRGB(resourceDesc.Format) : resourceDesc.Format;

	// TODO: Add 1D/3D support
	switch (resourceDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resourceDesc.DepthOrArraySize > 1)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mipSlice;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = arraySize;
			desc.Texture2DArray.PlaneSlice = 0;
		}
		else
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
			desc.Texture2D.PlaneSlice = 0;
		}
		break;

	default:
		break;
	}

	Device->CreateRenderTargetView(pResource, &desc, DestDescriptor.CpuHandle);
}

void RenderDevice::CreateDepthStencilView(ID3D12Resource* pResource,
	const Descriptor& DestDescriptor,
	std::optional<UINT> ArraySlice /*= {}*/,
	std::optional<UINT> MipSlice /*= {}*/,
	std::optional<UINT> ArraySize /*= {}*/)
{
	const auto resourceDesc = pResource->GetDesc();

	UINT arraySlice = ArraySlice.value_or(0);
	UINT mipSlice = MipSlice.value_or(0);
	UINT arraySize = ArraySize.value_or(resourceDesc.DepthOrArraySize);

	auto GetValidDSVFormat = [](DXGI_FORMAT Format)
	{
		// TODO: Add more
		switch (Format)
		{
		case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_D32_FLOAT;

		default: return DXGI_FORMAT_UNKNOWN;
		}
	};

	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = GetValidDSVFormat(resourceDesc.Format);
	desc.Flags = D3D12_DSV_FLAG_NONE;

	switch (resourceDesc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (resourceDesc.DepthOrArraySize > 1)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = mipSlice;
			desc.Texture2DArray.FirstArraySlice = arraySlice;
			desc.Texture2DArray.ArraySize = arraySize;
		}
		else
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = mipSlice;
		}
		break;

	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		throw std::invalid_argument("Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");

	default:
		break;
	}

	Device->CreateDepthStencilView(pResource, &desc, DestDescriptor.CpuHandle);
}

void RenderDevice::InitializeDXGIObjects()
{
	constexpr bool DEBUG_MODE = _DEBUG;

	constexpr UINT flags = DEBUG_MODE ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	ThrowIfFailed(::CreateDXGIFactory2(flags, IID_PPV_ARGS(m_DXGIFactory.ReleaseAndGetAddressOf())));

	// Check tearing support
	BOOL allowTearing = FALSE;
	if (FAILED(m_DXGIFactory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&allowTearing, sizeof(allowTearing))))
	{
		allowTearing = FALSE;
	}
	m_TearingSupport = allowTearing == TRUE;

	// Enumerate hardware for an adapter that supports DX12
	ComPtr<IDXGIAdapter4> pAdapter4;
	UINT adapterID = 0;
	while (m_DXGIFactory->EnumAdapterByGpuPreference(adapterID, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(pAdapter4.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND)
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
			m_DXGIAdapter = pAdapter4;
			m_AdapterDesc = desc;
			break;
		}

		adapterID++;
	}
}

void RenderDevice::InitializeDXGISwapChain()
{
	const Window& Window = Application::Window;

	// Create DXGISwapChain
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = Window.GetWindowWidth();
	desc.Height = Window.GetWindowHeight();
	desc.Format = SwapChainBufferFormat;
	desc.Stereo = FALSE;
	desc.SampleDesc = DefaultSampleDesc();
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = NumSwapChainBuffers;
	desc.Scaling = DXGI_SCALING_NONE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = m_TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	ComPtr<IDXGISwapChain1> pSwapChain1;
	ThrowIfFailed(m_DXGIFactory->CreateSwapChainForHwnd(GraphicsQueue, Window.GetWindowHandle(), &desc, nullptr, nullptr, pSwapChain1.ReleaseAndGetAddressOf()));
	ThrowIfFailed(m_DXGIFactory->MakeWindowAssociation(Window.GetWindowHandle(), DXGI_MWA_NO_ALT_ENTER)); // No full screen via alt + enter
	ThrowIfFailed(pSwapChain1->QueryInterface(IID_PPV_ARGS(m_DXGISwapChain.ReleaseAndGetAddressOf())));

	m_BackBufferIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
}

CommandQueue& RenderDevice::GetCommandQueue(D3D12_COMMAND_LIST_TYPE CommandListType)
{
	switch (CommandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: return GraphicsQueue;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE: return ComputeQueue;
	case D3D12_COMMAND_LIST_TYPE_COPY: return CopyQueue;
	default: return GraphicsQueue;
	}
}

void RenderDevice::AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder)
{
	/* Descriptor Tables */

	// ShaderResource
	RootDescriptorTable shaderResourceDescriptorTable;
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		shaderResourceDescriptorTable.AddDescriptorRange(DescriptorRange::Type::SRV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 100, Flags, 0)); // g_Texture2DTable
		shaderResourceDescriptorTable.AddDescriptorRange(DescriptorRange::Type::SRV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 101, Flags, 0)); // g_Texture2DUINT4Table
		shaderResourceDescriptorTable.AddDescriptorRange(DescriptorRange::Type::SRV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 102, Flags, 0)); // g_Texture2DArrayTable
		shaderResourceDescriptorTable.AddDescriptorRange(DescriptorRange::Type::SRV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 103, Flags, 0)); // g_TextureCubeTable
		shaderResourceDescriptorTable.AddDescriptorRange(DescriptorRange::Type::SRV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 104, Flags, 0)); // g_ByteAddressBufferTable
	}
	RootSignatureBuilder.AddRootDescriptorTableParameter(shaderResourceDescriptorTable);

	// UnorderedAccess
	RootDescriptorTable unorderedAccessDescriptorTable;
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		unorderedAccessDescriptorTable.AddDescriptorRange(DescriptorRange::Type::UAV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 100, Flags, 0)); // g_RWTexture2DTable
		unorderedAccessDescriptorTable.AddDescriptorRange(DescriptorRange::Type::UAV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 101, Flags, 0)); // g_RWTexture2DArrayTable
		unorderedAccessDescriptorTable.AddDescriptorRange(DescriptorRange::Type::UAV, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 102, Flags, 0)); // g_RWByteAddressBufferTable
	}
	RootSignatureBuilder.AddRootDescriptorTableParameter(unorderedAccessDescriptorTable);

	// Sampler
	RootDescriptorTable samplerDescriptorTable;
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		samplerDescriptorTable.AddDescriptorRange(DescriptorRange::Type::Sampler, DescriptorRange(RootSignature::UnboundDescriptorSize, 0, 100, Flags, 0)); // g_SamplerTable
	}
	RootSignatureBuilder.AddRootDescriptorTableParameter(samplerDescriptorTable);
}

void RenderDevice::ExecuteCommandListsInternal(D3D12_COMMAND_LIST_TYPE Type, UINT NumCommandLists, CommandList* ppCommandLists[])
{
	ScopedWriteLock _(m_GlobalResourceStateTrackerLock);

	std::vector<ID3D12CommandList*> commandlistsToBeExecuted;
	commandlistsToBeExecuted.reserve(size_t(NumCommandLists) * 2);
	for (UINT i = 0; i < NumCommandLists; ++i)
	{
		CommandList* pCommandList = ppCommandLists[i];
		if (pCommandList->Close(&m_GlobalResourceStateTracker))
		{
			commandlistsToBeExecuted.push_back(pCommandList->pPendingCommandList.Get());
		}
		commandlistsToBeExecuted.push_back(pCommandList->pCommandList.Get());
	}

	CommandQueue& CommandQueue = GetCommandQueue(Type);
	CommandQueue->ExecuteCommandLists(commandlistsToBeExecuted.size(), commandlistsToBeExecuted.data());
}