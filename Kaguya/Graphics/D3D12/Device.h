#pragma once
#include <Core/RWLock.h>

#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "Resource.h"

// clang-format off
template<D3D12_FEATURE Feature> struct FeatureStructMap									{ using Type = void; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS>							{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS; };
template<> struct FeatureStructMap<D3D12_FEATURE_ARCHITECTURE>							{ using Type = D3D12_FEATURE_DATA_ARCHITECTURE; };
template<> struct FeatureStructMap<D3D12_FEATURE_FEATURE_LEVELS>						{ using Type = D3D12_FEATURE_DATA_FEATURE_LEVELS; };
template<> struct FeatureStructMap<D3D12_FEATURE_FORMAT_SUPPORT>						{ using Type = D3D12_FEATURE_DATA_FORMAT_SUPPORT; };
template<> struct FeatureStructMap<D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS>			{ using Type = D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS; };
template<> struct FeatureStructMap<D3D12_FEATURE_FORMAT_INFO>							{ using Type = D3D12_FEATURE_DATA_FORMAT_INFO; };
template<> struct FeatureStructMap<D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT>			{ using Type = D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT; };
template<> struct FeatureStructMap<D3D12_FEATURE_SHADER_MODEL>							{ using Type = D3D12_FEATURE_DATA_SHADER_MODEL; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS1>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS1; };
template<> struct FeatureStructMap<D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT>	{ using Type = D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT; };
template<> struct FeatureStructMap<D3D12_FEATURE_ROOT_SIGNATURE>						{ using Type = D3D12_FEATURE_DATA_ROOT_SIGNATURE; };
template<> struct FeatureStructMap<D3D12_FEATURE_ARCHITECTURE1>							{ using Type = D3D12_FEATURE_DATA_ARCHITECTURE1; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS2>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS2; };
template<> struct FeatureStructMap<D3D12_FEATURE_SHADER_CACHE>							{ using Type = D3D12_FEATURE_DATA_SHADER_CACHE; };
template<> struct FeatureStructMap<D3D12_FEATURE_COMMAND_QUEUE_PRIORITY>				{ using Type = D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS3>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS3; };
template<> struct FeatureStructMap<D3D12_FEATURE_EXISTING_HEAPS>						{ using Type = D3D12_FEATURE_DATA_EXISTING_HEAPS; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS4>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS4; };
template<> struct FeatureStructMap<D3D12_FEATURE_SERIALIZATION>							{ using Type = D3D12_FEATURE_DATA_SERIALIZATION; };
template<> struct FeatureStructMap<D3D12_FEATURE_CROSS_NODE>							{ using Type = D3D12_FEATURE_DATA_CROSS_NODE; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS5>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS5; };
template<> struct FeatureStructMap<D3D12_FEATURE_D3D12_OPTIONS6>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS6; };
template<> struct FeatureStructMap<D3D12_FEATURE_QUERY_META_COMMAND>					{ using Type = D3D12_FEATURE_DATA_QUERY_META_COMMAND; };
// clang-format on

template<D3D12_FEATURE Feature>
struct D3D12Feature
{
	static constexpr D3D12_FEATURE Feature = Feature;
	using Type							   = FeatureStructMap<Feature>::Type;
	Type FeatureSupportData;

	const Type* operator->() const { return &FeatureSupportData; }
};

struct DeviceOptions
{
	D3D_FEATURE_LEVEL FeatureLevel;
	bool			  EnableDebugLayer;
	bool			  EnableGpuBasedValidation;
	bool			  BreakOnCorruption;
	bool			  BreakOnError;
	bool			  BreakOnWarning;
	bool			  EnableAutoDebugName;
};

// Can't use DeviceCapabilities, it conflicts with a macro
// Specify whether or not you want to require the device to have these features
struct DeviceFeatures
{
	bool WaveOperation;
	bool Raytracing;
	bool DynamicResources;
};

// I really want to do mGpu, gotta get myself another 2070 for mGpu
// or get 2 30 series, I can't get one because they're always out of stock >:(
class Device
{
public:
	static void ReportLiveObjects();

	Device() noexcept = default;
	Device(_In_ IDXGIAdapter4* pAdapter, const DeviceOptions& Options);
	~Device();

	void Initialize(const DeviceFeatures& Features);

	ID3D12Device*  GetDevice() const;
	ID3D12Device5* GetDevice5() const;

	CommandQueue* GetCommandQueue(ECommandQueueType Type);
	CommandQueue* GetGraphicsQueue() { return GetCommandQueue(ECommandQueueType::Direct); }
	CommandQueue* GetAsyncComputeQueue() { return GetCommandQueue(ECommandQueueType::AsyncCompute); }
	CommandQueue* GetCopyQueue1() { return GetCommandQueue(ECommandQueueType::Copy1); }
	CommandQueue* GetCopyQueue2() { return GetCommandQueue(ECommandQueueType::Copy2); }

	// clang-format off
	template <typename TViewDesc> DescriptorHeap& GetDescriptorHeap();
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return RenderTargetDescriptorHeap; }
	template<> DescriptorHeap& GetDescriptorHeap<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return DepthStencilDescriptorHeap; }
	// clang-format on

	DescriptorHeap& GetResourceDescriptorHeap() noexcept { return ResourceDescriptorHeap; }
	DescriptorHeap& GetSamplerDescriptorHeap() noexcept { return SamplerDescriptorHeap; }
	DescriptorHeap& GetRenderTargetDescriptorHeap() noexcept { return RenderTargetDescriptorHeap; }
	DescriptorHeap& GetDepthStencilDescriptorHeap() noexcept { return DepthStencilDescriptorHeap; }

	CommandContext& GetCommandContext(UINT ThreadIndex = 0) { return *AvailableCommandContexts[ThreadIndex]; }
	CommandContext& GetAsyncComputeCommandContext(UINT ThreadIndex = 0)
	{
		return *AvailableAsyncCommandContexts[ThreadIndex];
	}
	CommandContext& GetCopyContext1() { return *CopyContext1; }
	CommandContext& GetCopyContext2() { return *CopyContext2; }

	D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& ResourceDesc);

	bool ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& ResourceDesc);

private:
	static void OnDeviceRemoved(PVOID Context, BOOLEAN);

private:
	Microsoft::WRL::ComPtr<ID3D12Device>  pDevice;
	Microsoft::WRL::ComPtr<ID3D12Device5> pDevice5;

	Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
	HANDLE								DeviceRemovedWaitHandle;
	wil::unique_event					DeviceRemovedEvent;

	CommandQueue GraphicsQueue;
	CommandQueue AsyncComputeQueue;
	CommandQueue CopyQueue1;
	CommandQueue CopyQueue2;

	DescriptorHeap ResourceDescriptorHeap;
	DescriptorHeap SamplerDescriptorHeap;

	DescriptorHeap RenderTargetDescriptorHeap;
	DescriptorHeap DepthStencilDescriptorHeap;

	std::vector<std::unique_ptr<CommandContext>> AvailableCommandContexts;
	std::vector<std::unique_ptr<CommandContext>> AvailableAsyncCommandContexts;

	std::unique_ptr<CommandContext> CopyContext1;
	std::unique_ptr<CommandContext> CopyContext2;

	RWLock													   ResourceAllocationInfoTableLock;
	std::unordered_map<UINT64, D3D12_RESOURCE_ALLOCATION_INFO> ResourceAllocationInfoTable;
};

template<typename ViewDesc>
void Descriptor<ViewDesc>::Allocate()
{
	if (Parent)
	{
		DescriptorHeap& Heap = Parent->GetDescriptorHeap<ViewDesc>();
		Heap.Allocate(CPUHandle, GPUHandle, Index);
	}
}

template<typename ViewDesc>
void Descriptor<ViewDesc>::Release()
{
	if (Parent && IsValid())
	{
		DescriptorHeap& Heap = Parent->GetDescriptorHeap<ViewDesc>();
		Heap.Release(Index);
		CPUHandle = { NULL };
		GPUHandle = { NULL };
		Index	  = UINT_MAX;
	}
}
