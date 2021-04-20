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
	~Device();

	Device(Device&&) noexcept = default;
	Device& operator=(Device&&) noexcept = default;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

	operator ID3D12Device5* () const { return m_Device.Get(); }
	ID3D12Device5* operator->() const { return m_Device.Get(); }

	void CreateAllocator(
		_In_ IDXGIAdapter4* pAdapter);

	D3D12MA::Allocator* GetAllocator() const noexcept { return m_Allocator; }
	DirectX::GraphicsMemory* GetGraphicsMemory() const noexcept { return m_GraphicsMemory.get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device;
	D3D12MA::Allocator* m_Allocator = nullptr;
	std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory;
};
