#pragma once
#include "D3D12Common.h"
#include "Device.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "RaytracingPipelineState.h"

class Device;

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

// Specify whether or not you want to require the device to have these features
struct DeviceFeatures
{
	D3D_FEATURE_LEVEL FeatureLevel;
	bool			  WaveOperation;
	bool			  Raytracing;
	bool			  DynamicResources;
};

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

class Adapter
{
public:
	static void ReportLiveObjects();

	Adapter();
	~Adapter();

	void Initialize(const DeviceOptions& Options);
	void InitializeDevice(const DeviceFeatures& Features);

	void OnBeginFrame() { Profiler.OnBeginFrame(); }
	void OnEndFrame() { Profiler.OnEndFrame(); }

	IDXGIFactory6* GetFactory6() const noexcept { return Factory6.Get(); }

	IDXGIAdapter4* GetAdapter4() const noexcept { return Adapter4.Get(); }

	ID3D12Device*  GetD3D12Device() const noexcept { return D3D12Device.Get(); }
	ID3D12Device5* GetD3D12Device5() const noexcept { return D3D12Device5.Get(); }

	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept
	{
		return DescriptorHandleIncrementSizeCache[Type];
	}

	void RegisterMessageCallback(D3D12MessageFunc CallbackFunc)
	{
		if (D3D12InfoQueue1)
		{
			ASSERTD3D12APISUCCEEDED(D3D12InfoQueue1->RegisterMessageCallback(
				CallbackFunc,
				D3D12_MESSAGE_CALLBACK_FLAG_NONE,
				nullptr,
				&CallbackCookie));
		}
	}

	void UnregisterMessageCallback()
	{
		if (D3D12InfoQueue1)
		{
			ASSERTD3D12APISUCCEEDED(D3D12InfoQueue1->UnregisterMessageCallback(CallbackCookie));
		}
	}

	Device* GetDevice() noexcept { return &Device; }

	[[nodiscard]] RootSignature CreateRootSignature(
		std::function<void(RootSignatureBuilder&)> Configurator,
		bool									   AddDescriptorTableRootParameters = true);

	[[nodiscard]] PipelineState CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& Desc)
	{
		return PipelineState(GetD3D12Device(), &Desc);
	}

	[[nodiscard]] PipelineState CreateComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC& Desc)
	{
		return PipelineState(GetD3D12Device(), &Desc);
	}

	template<typename PipelineStateStream>
	[[nodiscard]] PipelineState CreatePipelineState(PipelineStateStream& Stream)
	{
		return PipelineState(GetD3D12Device5(), Stream);
	}

	[[nodiscard]] RaytracingPipelineState CreateRaytracingPipelineState(
		std::function<void(RaytracingPipelineStateBuilder&)> Configurator);

	void BindGraphicsDescriptorTable(const RootSignature& RootSignature, CommandContext& Context)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor = GetDevice()->GetResourceDescriptorHeap().hGPU(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor  = GetDevice()->GetSamplerDescriptorHeap().hGPU(0);

		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::SamplerDescriptorTable + Offset,
			SamplerDescriptor);
	}

	void BindComputeDescriptorTable(const RootSignature& RootSignature, CommandContext& Context)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor = GetDevice()->GetResourceDescriptorHeap().hGPU(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor  = GetDevice()->GetSamplerDescriptorHeap().hGPU(0);

		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::SamplerDescriptorTable + Offset,
			SamplerDescriptor);
	}

private:
	static void OnDeviceRemoved(PVOID Context, BOOLEAN);

	void InitializeDXGIObjects(bool Debug);

	void AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory6> Factory6;

	AftermathCrashTracker AftermathCrashTracker;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter4;
	DXGI_ADAPTER_DESC3					  AdapterDesc = {};

	Microsoft::WRL::ComPtr<ID3D12Device>  D3D12Device;
	Microsoft::WRL::ComPtr<ID3D12Device5> D3D12Device5;

	UINT DescriptorHandleIncrementSizeCache[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> D3D12InfoQueue1;
	DWORD									 CallbackCookie = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
	HANDLE								DeviceRemovedWaitHandle = nullptr;
	wil::unique_event					DeviceRemovedEvent;

	Device Device;

	Profiler Profiler;
};
