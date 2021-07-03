#include "pch.h"
#include "RaytracingShaderTable.h"

void RaytracingShaderBindingTable::Generate(Device* Device)
{
	RayGenerationShaderTableOffset = SizeInBytes;
	SizeInBytes += RayGenerationShaderTable->GetTotalSizeInBytes();
	SizeInBytes = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	MissShaderTableOffset = SizeInBytes;
	SizeInBytes += MissShaderTable->GetTotalSizeInBytes();
	SizeInBytes = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	HitGroupShaderTableOffset = SizeInBytes;
	SizeInBytes += HitGroupShaderTable->GetTotalSizeInBytes();
	SizeInBytes = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	SBTBuffer = Buffer(Device, SizeInBytes, 0, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	CPUData	  = std::make_unique<BYTE[]>(SizeInBytes);
}

void RaytracingShaderBindingTable::Write()
{
	BYTE* BaseAddress = CPUData.get();

	RayGenerationShaderTable->Write(BaseAddress + RayGenerationShaderTableOffset);
	MissShaderTable->Write(BaseAddress + MissShaderTableOffset);
	HitGroupShaderTable->Write(BaseAddress + HitGroupShaderTableOffset);
}

void RaytracingShaderBindingTable::CopyToGPU(CommandContext& Context) const
{
	Allocation Allocation = Context.CpuConstantAllocator.Allocate(SizeInBytes);
	std::memcpy(Allocation.CPUVirtualAddress, CPUData.get(), SizeInBytes);

	Context->CopyBufferRegion(SBTBuffer.GetResource(), 0, Allocation.pResource, Allocation.Offset, Allocation.Size);
}

D3D12_DISPATCH_RAYS_DESC RaytracingShaderBindingTable::GetDispatchRaysDesc(
	UINT RayGenerationShaderIndex,
	UINT BaseMissShaderIndex) const
{
	const UINT64 RayGenerationShaderTableSizeInBytes = RayGenerationShaderTable->GetSizeInBytes();
	const UINT64 MissShaderTableSizeInBytes			 = MissShaderTable->GetSizeInBytes();
	const UINT64 HitGroupShaderTableSizeInBytes		 = HitGroupShaderTable->GetSizeInBytes();

	const UINT64 RayGenerationShaderRecordStride = RayGenerationShaderTable->GetStrideInBytes();
	const UINT64 MissShaderRecordStride			 = MissShaderTable->GetStrideInBytes();
	const UINT64 HitGroupShaderRecordStride		 = HitGroupShaderTable->GetStrideInBytes();

	D3D12_GPU_VIRTUAL_ADDRESS BaseAddress = SBTBuffer.GetGPUVirtualAddress();

	D3D12_DISPATCH_RAYS_DESC Desc = {};

	Desc.RayGenerationShaderRecord = { .StartAddress = BaseAddress + RayGenerationShaderTableOffset +
													   RayGenerationShaderIndex * RayGenerationShaderRecordStride,
									   .SizeInBytes = RayGenerationShaderTableSizeInBytes };

	Desc.MissShaderTable = { .StartAddress =
								 BaseAddress + MissShaderTableOffset + BaseMissShaderIndex * MissShaderRecordStride,
							 .SizeInBytes	= MissShaderTableSizeInBytes,
							 .StrideInBytes = MissShaderRecordStride };

	Desc.HitGroupTable = { .StartAddress  = BaseAddress + HitGroupShaderTableOffset,
						   .SizeInBytes	  = HitGroupShaderTableSizeInBytes,
						   .StrideInBytes = HitGroupShaderRecordStride };

	return Desc;
}
