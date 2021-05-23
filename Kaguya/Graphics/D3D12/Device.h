#pragma once

struct DeviceOptions
{
	D3D_FEATURE_LEVEL FeatureLevel;
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool BreakOnCorruption;
	bool BreakOnError;
	bool BreakOnWarning;
	bool EnableAutoDebugName;
};

// Can't use DeviceCapabilities, it conflicts with a macro
// Specify whether or not you want to require the device to have these features
// if any is not supported, ctor throws
struct DeviceFeatures
{
	bool WaveOperation;
	bool Raytracing;
	bool DynamicResources;
};

class Device
{
public:
	static void ReportLiveObjects();

	Device() noexcept = default;
	Device(
		_In_ IDXGIAdapter4* pAdapter,
		_In_ const DeviceOptions& Options,
		_In_ const DeviceFeatures& Features);
	~Device();

	Device(Device&&) noexcept = default;
	Device& operator=(Device&&) noexcept = default;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

	operator ID3D12Device5* () const { return m_Device.Get(); }
	ID3D12Device5* operator->() const { return m_Device.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
};
