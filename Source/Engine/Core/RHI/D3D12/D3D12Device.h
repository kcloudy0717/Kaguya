#pragma once
#include <Core/RHI/RHICommon.h>
#include "D3D12Common.h"
#include "D3D12LinkedDevice.h"
#include "D3D12RenderPass.h"
#include "D3D12RenderTarget.h"
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
	bool			  MeshShaders;
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

	[[nodiscard]] auto GetDxgiFactory6() const noexcept -> IDXGIFactory6* { return Factory6.Get(); }
	[[nodiscard]] auto GetDxgiAdapter3() const noexcept -> IDXGIAdapter3* { return Adapter3.Get(); }
	[[nodiscard]] auto GetD3D12Device() const noexcept -> ID3D12Device* { return Device.Get(); }
	[[nodiscard]] auto GetD3D12Device5() const noexcept -> ID3D12Device5* { return Device5.Get(); }

	[[nodiscard]] UINT GetSizeOfDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept
	{
		return DescriptorSizeCache[Type];
	}

	[[nodiscard]] D3D12LinkedDevice* GetDevice() noexcept { return &LinkedDevice; }

	[[nodiscard]] bool		  AllowAsyncPsoCompilation() const noexcept;
	[[nodiscard]] ThreadPool* GetPsoCompilationThreadPool() const noexcept { return PsoCompilationThreadPool.get(); }

	[[nodiscard]] std::unique_ptr<D3D12RootSignature> CreateRootSignature(
		Delegate<void(RootSignatureBuilder&)> Configurator);

	template<typename PipelineStateStream>
	[[nodiscard]] std::unique_ptr<D3D12PipelineState> CreatePipelineState(PipelineStateStream& Stream)
	{
		PipelineStateStreamDesc Desc;
		Desc.SizeInBytes				   = sizeof(Stream);
		Desc.pPipelineStateSubobjectStream = &Stream;
		return std::make_unique<D3D12PipelineState>(this, Desc);
	}

	[[nodiscard]] D3D12RaytracingPipelineState CreateRaytracingPipelineState(
		Delegate<void(RaytracingPipelineStateBuilder&)> Configurator);

private:
	static void OnDeviceRemoved(PVOID Context, BOOLEAN);

	void InitializeDxgiObjects(bool Debug);

	// Pre SM6.6 bindless root parameter setup
	void AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory6> Factory6;

	Microsoft::WRL::ComPtr<IDXGIAdapter3> Adapter3;
	DXGI_ADAPTER_DESC3					  AdapterDesc = {};

	Microsoft::WRL::ComPtr<ID3D12Device>  Device;
	Microsoft::WRL::ComPtr<ID3D12Device5> Device5;

	CD3DX12FeatureSupport FeatureSupport;

	UINT DescriptorSizeCache[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> InfoQueue1;
	DWORD									 CallbackCookie = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
	HANDLE								DeviceRemovedWaitHandle = nullptr;
	wil::unique_event					DeviceRemovedEvent;

	// Represents a single node
	// TODO: Add Multi-Adapter support
	D3D12LinkedDevice LinkedDevice;

	D3D12Profiler Profiler;

	AftermathCrashTracker AftermathCrashTracker;

	std::unique_ptr<ThreadPool> PsoCompilationThreadPool;
};
