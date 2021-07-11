#pragma once
#include "D3D12Common.h"
#include "Device.h"

class Device;

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

// Can't use DeviceCapabilities, it conflicts with a macro
// Specify whether or not you want to require the device to have these features
struct DeviceFeatures
{
	D3D_FEATURE_LEVEL FeatureLevel;
	bool			  WaveOperation;
	bool			  Raytracing;
	bool			  DynamicResources;
};

class Adapter
{
public:
	static void ReportLiveObjects();

	Adapter();
	~Adapter();

	void Initialize(const DeviceOptions& Options);
	void InitializeDevice(const DeviceFeatures& Features);

	IDXGIFactory6* GetFactory6() const { return Factory6.Get(); }

	IDXGIAdapter4* GetAdapter4() const;

	ID3D12Device*  GetD3D12Device() const;
	ID3D12Device5* GetD3D12Device5() const;

	Device* GetDevice() { return &Device; }

private:
	static void OnDeviceRemoved(PVOID Context, BOOLEAN);

	void InitializeDXGIObjects(bool Debug);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory6> Factory6;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter4;
	DXGI_ADAPTER_DESC3					  AdapterDesc = {};

	Microsoft::WRL::ComPtr<ID3D12Device>  D3D12Device;
	Microsoft::WRL::ComPtr<ID3D12Device5> D3D12Device5;

	Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
	HANDLE								DeviceRemovedWaitHandle = nullptr;
	wil::unique_event					DeviceRemovedEvent;

	Device Device;
};
