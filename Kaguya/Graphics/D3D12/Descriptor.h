#pragma once

struct Descriptor
{
	__forceinline bool IsValid() const noexcept
	{
		return CpuHandle.ptr != NULL;
	}

	__forceinline bool IsReferencedByShader() const noexcept
	{
		return GpuHandle.ptr != NULL;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { NULL };
	UINT Index = 0;
};
