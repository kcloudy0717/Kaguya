#pragma once
#include <Core/RHI/RHICommon.h>
#include "D3D12Common.h"
#include "D3D12LinkedDevice.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "D3D12RaytracingPipelineState.h"

class D3D12LinkedDevice;

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

class D3D12Device
{
public:
	static void ReportLiveObjects();

	D3D12Device();
	~D3D12Device();

	void Initialize(const DeviceOptions& Options);
	void InitializeDevice(const DeviceFeatures& Features);

	void OnBeginFrame() { Profiler.OnBeginFrame(); }
	void OnEndFrame() { Profiler.OnEndFrame(); }

	auto GetFactory6() const noexcept -> IDXGIFactory6* { return Factory6.Get(); }
	auto GetAdapter4() const noexcept -> IDXGIAdapter4* { return Adapter4.Get(); }
	auto GetD3D12Device() const noexcept -> ID3D12Device* { return Device.Get(); }
	auto GetD3D12Device5() const noexcept -> ID3D12Device5* { return Device5.Get(); }

	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept
	{
		return DescriptorHandleIncrementSizeCache[Type];
	}

	void RegisterMessageCallback(D3D12MessageFunc CallbackFunc)
	{
		if (InfoQueue1)
		{
			VERIFY_D3D12_API(InfoQueue1->RegisterMessageCallback(
				CallbackFunc,
				D3D12_MESSAGE_CALLBACK_FLAG_NONE,
				nullptr,
				&CallbackCookie));
		}
	}

	void UnregisterMessageCallback()
	{
		if (InfoQueue1)
		{
			VERIFY_D3D12_API(InfoQueue1->UnregisterMessageCallback(CallbackCookie));
		}
	}

	D3D12LinkedDevice* GetDevice() noexcept { return &LinkedDevice; }

	[[nodiscard]] D3D12RootSignature CreateRootSignature(
		std::function<void(RootSignatureBuilder&)> Configurator,
		bool									   AddDescriptorTableRootParameters = true);

	[[nodiscard]] D3D12PipelineState CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& Desc)
	{
		return D3D12PipelineState(GetD3D12Device(), &Desc);
	}

	[[nodiscard]] D3D12PipelineState CreateComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC& Desc)
	{
		return D3D12PipelineState(GetD3D12Device(), &Desc);
	}

	template<typename PipelineStateStream>
	[[nodiscard]] D3D12PipelineState CreatePipelineState(PipelineStateStream& Stream)
	{
		return D3D12PipelineState(GetD3D12Device5(), Stream);
	}

	[[nodiscard]] D3D12RaytracingPipelineState CreateRaytracingPipelineState(
		std::function<void(RaytracingPipelineStateBuilder&)> Configurator);

	void BindGraphicsDescriptorTable(const D3D12RootSignature& RootSignature, D3D12CommandContext& Context)
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

	void BindComputeDescriptorTable(const D3D12RootSignature& RootSignature, D3D12CommandContext& Context)
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

	Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter4;
	DXGI_ADAPTER_DESC3					  AdapterDesc = {};

	Microsoft::WRL::ComPtr<ID3D12Device>  Device;
	Microsoft::WRL::ComPtr<ID3D12Device5> Device5;

	UINT DescriptorHandleIncrementSizeCache[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> InfoQueue1;
	DWORD									 CallbackCookie = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
	HANDLE								DeviceRemovedWaitHandle = nullptr;
	wil::unique_event					DeviceRemovedEvent;

	D3D12LinkedDevice LinkedDevice;

	D3D12Profiler Profiler;

	AftermathCrashTracker AftermathCrashTracker;
};
