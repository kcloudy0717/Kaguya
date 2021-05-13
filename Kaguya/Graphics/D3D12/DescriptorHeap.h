#pragma once
#include "Descriptor.h"

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

	Descriptor At(
		_In_ UINT Index) const noexcept;

	Descriptor GetDescriptorFromStart() const noexcept
	{
		return m_StartDescriptor;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	Descriptor m_StartDescriptor = {};
	UINT m_DescriptorIncrementSize = 0;
};
