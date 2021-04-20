#pragma once

struct Descriptor
{
	inline bool IsValid() const { return CpuHandle.ptr != NULL; }
	inline bool IsReferencedByShader() const { return GpuHandle.ptr != NULL; }

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { NULL };
	UINT Index = 0;
};

class DescriptorHeap
{
public:
	DescriptorHeap() = default;
	DescriptorHeap(
		_In_ ID3D12Device* pDevice,
		_In_ UINT NumDescriptors,
		_In_ bool ShaderVisible,
		_In_ D3D12_DESCRIPTOR_HEAP_TYPE Type);

	DescriptorHeap(DescriptorHeap&&) noexcept = default;
	DescriptorHeap& operator=(DescriptorHeap&&) noexcept = default;

	DescriptorHeap(const DescriptorHeap&) = delete;
	DescriptorHeap& operator=(const DescriptorHeap&) = delete;

	operator ID3D12DescriptorHeap* () const { return m_DescriptorHeap.Get(); }
	ID3D12DescriptorHeap* operator->() const { return m_DescriptorHeap.Get(); }

	Descriptor GetDescriptorFromStart() const { return m_StartDescriptor; }

	Descriptor At(
		_In_ UINT Index) const;

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Descriptor m_StartDescriptor = {};
	UINT m_DescriptorIncrementSize = 0;
};
