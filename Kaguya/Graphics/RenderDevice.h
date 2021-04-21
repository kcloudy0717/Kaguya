#pragma once
#include <GraphicsMemory.h>

#include "D3D12/Device.h"
#include "D3D12/CommandQueue.h"
#include "D3D12/ResourceViewHeaps.h"
#include "D3D12/Fence.h"
#include "D3D12/ResourceStateTracker.h"
#include "D3D12/ShaderCompiler.h"
#include "D3D12/CommandList.h"
#include "D3D12/AccelerationStructure.h"
#include "D3D12/ShaderTable.h"
#include "D3D12/RootSignature.h"
#include "D3D12/RaytracingPipelineState.h"

struct RootParameters
{
	struct DescriptorTable
	{
		enum
		{
			ShaderResourceDescriptorTable,
			UnorderedAccessDescriptorTable,
			SamplerDescriptorTable,
			NumRootParameters
		};
	};
};

struct Resource
{
	~Resource();

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	D3D12MA::Allocation* pAllocation = nullptr;
};

class RenderDevice
{
public:
	enum
	{
		NumSwapChainBuffers = 3,
	};

	static constexpr DXGI_FORMAT SwapChainBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;
	static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static void Initialize();
	static void Shutdown();
	static RenderDevice& Instance();

	void Present(bool VSync);

	void Resize(UINT Width, UINT Height);

	void BindResourceViewHeaps(CommandList& CommandList);

	void BindGraphicsDescriptorTable(const RootSignature& RootSignature, CommandList& CommandList)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const auto rootParameterOffset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		auto resourceDescriptorFromStart = m_ResourceViewHeaps.ResourceDescriptorHeap().GetDescriptorFromStart();
		auto samplerDescriptorFromStart = m_ResourceViewHeaps.SamplerDescriptorHeap().GetDescriptorFromStart();

		CommandList->SetGraphicsRootDescriptorTable(RootParameters::DescriptorTable::ShaderResourceDescriptorTable + rootParameterOffset, resourceDescriptorFromStart.GpuHandle);
		CommandList->SetGraphicsRootDescriptorTable(RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + rootParameterOffset, resourceDescriptorFromStart.GpuHandle);
		CommandList->SetGraphicsRootDescriptorTable(RootParameters::DescriptorTable::SamplerDescriptorTable + rootParameterOffset, samplerDescriptorFromStart.GpuHandle);
	}

	void BindComputeDescriptorTable(const RootSignature& RootSignature, CommandList& CommandList)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const auto rootParameterOffset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		auto resourceDescriptorFromStart = m_ResourceViewHeaps.ResourceDescriptorHeap().GetDescriptorFromStart();
		auto samplerDescriptorFromStart = m_ResourceViewHeaps.SamplerDescriptorHeap().GetDescriptorFromStart();

		CommandList->SetComputeRootDescriptorTable(RootParameters::DescriptorTable::ShaderResourceDescriptorTable + rootParameterOffset, resourceDescriptorFromStart.GpuHandle);
		CommandList->SetComputeRootDescriptorTable(RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + rootParameterOffset, resourceDescriptorFromStart.GpuHandle);
		CommandList->SetComputeRootDescriptorTable(RootParameters::DescriptorTable::SamplerDescriptorTable + rootParameterOffset, samplerDescriptorFromStart.GpuHandle);
	}

	void ExecuteGraphicsContexts(UINT NumCommandLists, CommandList* ppCommandLists[]) { ExecuteCommandListsInternal(D3D12_COMMAND_LIST_TYPE_DIRECT, NumCommandLists, ppCommandLists); }
	void ExecuteAsyncComputeContexts(UINT NumCommandLists, CommandList* ppCommandLists[]) { ExecuteCommandListsInternal(D3D12_COMMAND_LIST_TYPE_COMPUTE, NumCommandLists, ppCommandLists); }
	void ExecuteCopyContexts(UINT NumCommandLists, CommandList* ppCommandLists[]) { ExecuteCommandListsInternal(D3D12_COMMAND_LIST_TYPE_COPY, NumCommandLists, ppCommandLists); }

	// Resource creation
	[[nodiscard]] std::shared_ptr<Resource> CreateResource(
		const D3D12MA::ALLOCATION_DESC* pAllocDesc,
		const D3D12_RESOURCE_DESC* pResourceDesc,
		D3D12_RESOURCE_STATES InitialResourceState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue);

	[[nodiscard]] std::shared_ptr<Resource> CreateBuffer(
		const D3D12MA::ALLOCATION_DESC* pAllocDesc,
		UINT64 Width,
		D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE,
		UINT64 Alignment = 0,
		D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_COMMON)
	{
		auto Desc = CD3DX12_RESOURCE_DESC::Buffer(Width, Flags, Alignment);
		return CreateResource(pAllocDesc, &Desc, InitialResourceState, nullptr);
	}

	[[nodiscard]] RootSignature CreateRootSignature(
		std::function<void(RootSignatureBuilder&)> Configurator,
		bool AddDescriptorTableRootParameters = true);

	template<typename PipelineStateStream>
	[[nodiscard]] PipelineState CreatePipelineState(
		PipelineStateStream& Stream)
	{
		return PipelineState(m_Device, Stream);
	}

	[[nodiscard]] RaytracingPipelineState CreateRaytracingPipelineState(
		std::function<void(RaytracingPipelineStateBuilder&)> Configurator);

	// Thread-Safe
	// Buffer variation
	void CreateShaderResourceView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		UINT NumElements,
		UINT Stride,
		bool IsRawBuffer = false);

	template<typename T>
	void CreateShaderResourceView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		UINT NumElements)
	{
		return CreateShaderResourceView(pResource, DestDescriptor, NumElements, sizeof(T));
	}

	// Texture variation
	void CreateShaderResourceView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		std::optional<UINT> MostDetailedMip = {},
		std::optional<UINT> MipLevels = {});

	void CreateUnorderedAccessView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		std::optional<UINT> ArraySlice = {},
		std::optional<UINT> MipSlice = {});

	void CreateRenderTargetView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		std::optional<UINT> ArraySlice = {},
		std::optional<UINT> MipSlice = {},
		std::optional<UINT> ArraySize = {},
		bool sRGB = false);

	void CreateDepthStencilView(
		ID3D12Resource* pResource,
		const Descriptor& DestDescriptor,
		std::optional<UINT> ArraySlice = {},
		std::optional<UINT> MipSlice = {},
		std::optional<UINT> ArraySize = {});

	const auto& GetAdapterDesc() const { return m_AdapterDesc; }
	DXGI_QUERY_VIDEO_MEMORY_INFO QueryLocalVideoMemoryInfo() const;

	Descriptor GetCurrentBackBufferRenderTargetView() const
	{
		UINT index = m_SwapChain->GetCurrentBackBufferIndex();
		return m_SwapChainBufferDescriptors[index];
	}

	ID3D12Resource* GetCurrentBackBuffer() const
	{
		UINT index = m_SwapChain->GetCurrentBackBufferIndex();

		ID3D12Resource* pBackBuffer = nullptr;
		ThrowIfFailed(m_SwapChain->GetBuffer(index, IID_PPV_ARGS(&pBackBuffer)));
		pBackBuffer->Release();
		return pBackBuffer;
	}

	Device& GetDevice() noexcept { return m_Device; }
	DirectX::GraphicsMemory* GraphicsMemory() const noexcept { return m_GraphicsMemory.get(); }

	ResourceViewHeaps& GetResourceViewHeaps() noexcept { return m_ResourceViewHeaps; }

	CommandQueue& GetGraphicsQueue() noexcept { return m_GraphicsQueue; }
	CommandQueue& GetComputeQueue() noexcept { return m_ComputeQueue; }
	CommandQueue& GetCopyQueue() noexcept { return m_CopyQueue; }

private:
	RenderDevice();
	~RenderDevice();

	RenderDevice(const RenderDevice&) = delete;
	RenderDevice& operator=(const RenderDevice&) = delete;

	void InitializeDXGIObjects();

	void InitializeDXGISwapChain();

	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE CommandListType);
	void AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder);

	void ExecuteCommandListsInternal(D3D12_COMMAND_LIST_TYPE Type, UINT NumCommandLists, CommandList* ppCommandLists[]);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory6> m_Factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
	DXGI_ADAPTER_DESC3 m_AdapterDesc;
	bool m_TearingSupport;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;

	Device m_Device;

	std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory;
	D3D12MA::Allocator* m_Allocator = nullptr;

	CommandQueue m_GraphicsQueue;
	CommandQueue m_ComputeQueue;
	CommandQueue m_CopyQueue;

	ResourceViewHeaps m_ResourceViewHeaps;

	Descriptor m_ImGuiDescriptor;
	Descriptor m_SwapChainBufferDescriptors[NumSwapChainBuffers];

	// Global resource state tracker
	ResourceStateTracker m_GlobalResourceStateTracker;
	CriticalSection m_GlobalResourceStateCriticalSection;
};
