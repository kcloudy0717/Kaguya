#include "pch.h"
#include "DescriptorHeap.h"
#include "d3dx12.h"

DescriptorHeap::DescriptorHeap(
	_In_ ID3D12Device* pDevice,
	_In_ UINT NumDescriptors,
	_In_ bool ShaderVisible,
	_In_ D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	// If you recorded a CPU descriptor handle into the command list (render target or depth stencil) then that descriptor can be reused immediately after the Set call, 
	// if you recorded a GPU descriptor handle into the command list (everything else) then that descriptor cannot be reused until gpu is done referencing them
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = Type,
		.NumDescriptors = NumDescriptors,
		.Flags = ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0 };

	ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
	m_DescriptorIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Type);

	D3D12_CPU_DESCRIPTOR_HANDLE startCpuHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE startGpuHandle = ShaderVisible ? m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{};
	m_StartDescriptor = { startCpuHandle, startGpuHandle, 0 };
}

Descriptor DescriptorHeap::At(
	_In_ UINT Index) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(cpuHandle, m_StartDescriptor.CpuHandle, Index, m_DescriptorIncrementSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE::InitOffsetted(gpuHandle, m_StartDescriptor.GpuHandle, Index, m_DescriptorIncrementSize);

	return { cpuHandle, gpuHandle, Index };
}
