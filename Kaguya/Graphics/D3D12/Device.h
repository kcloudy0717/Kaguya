#pragma once

struct DeviceOptions
{
	D3D_FEATURE_LEVEL FeatureLevel;
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool BreakOnCorruption;
	bool BreakOnError;
	bool BreakOnWarning;
};

class Device
{
public:
	static void ReportLiveObjects();

	Device() noexcept = default;
	explicit Device(
		_In_ IDXGIAdapter4* pAdapter,
		_In_ const DeviceOptions& Options);

	Device(Device&&) noexcept = default;
	Device& operator=(Device&&) noexcept = default;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

	operator ID3D12Device5* () const { return m_Device5.Get(); }
	ID3D12Device5* operator->() { return m_Device5.Get(); }

private:
	void CheckRootSignature_1_1Support();
	void CheckShaderModel6PlusSupport();
	void CheckRaytracingSupport();
	void CheckMeshShaderSupport();

private:
	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device5;
};
