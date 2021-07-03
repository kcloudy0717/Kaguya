#include "pch.h"
#include "DescriptorHeap.h"
#include "Device.h"
#include "d3dx12.h"

void DescriptorHeap::Create(UINT NumDescriptors, bool ShaderVisible, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	// If you recorded a CPU descriptor handle into the command list (render target or depth stencil) then that
	// descriptor can be reused immediately after the Set call, if you recorded a GPU descriptor handle into the command
	// list (everything else) then that descriptor cannot be reused until gpu is done referencing them
	Desc = { .Type			 = Type,
			 .NumDescriptors = NumDescriptors,
			 .Flags	   = ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			 .NodeMask = 0 };

	ThrowIfFailed(GetParentDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pDescriptorHeap)));
	DescriptorStride = GetParentDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	hCPUStart = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	hGPUStart = ShaderVisible ? pDescriptorHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE();

	IndexPool = { NumDescriptors };
}

void DescriptorHeap::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& hCPU, D3D12_GPU_DESCRIPTOR_HANDLE& hGPU, UINT& Index)
{
	std::scoped_lock _(Mutex);

	Index = static_cast<UINT>(IndexPool.Allocate());
	hCPU  = this->hCPU(Index);
	hGPU  = this->hGPU(Index);
}

void DescriptorHeap::Release(UINT Index)
{
	std::scoped_lock _(Mutex);

	IndexPool.Release(static_cast<size_t>(Index));
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::hCPU(UINT Index) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE hCPU = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(hCPU, hCPUStart, Index, DescriptorStride);
	return hCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::hGPU(UINT Index) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE::InitOffsetted(hGPU, hGPUStart, Index, DescriptorStride);
	return hGPU;
}
