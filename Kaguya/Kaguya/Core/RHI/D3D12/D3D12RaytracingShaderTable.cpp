#include "D3D12RaytracingShaderTable.h"

void D3D12RaytracingShaderBindingTable::Generate(D3D12LinkedDevice* Device)
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

	SBTBuffer		= D3D12Buffer(Device, SizeInBytes, 0, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	SBTUploadBuffer = D3D12Buffer(Device, SizeInBytes, 0, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
	SBTUploadBuffer.Initialize();
	CpuData = std::make_unique<BYTE[]>(SizeInBytes);
}

void D3D12RaytracingShaderBindingTable::Write()
{
	BYTE* BaseAddress = CpuData.get();

	RayGenerationShaderTable->Write(BaseAddress + RayGenerationShaderTableOffset);
	MissShaderTable->Write(BaseAddress + MissShaderTableOffset);
	HitGroupShaderTable->Write(BaseAddress + HitGroupShaderTableOffset);
}

void D3D12RaytracingShaderBindingTable::CopyToGpu(D3D12CommandContext& Context) const
{
	BYTE* CPUVirtualAddress = SBTUploadBuffer.GetCpuVirtualAddress<BYTE>();
	std::memcpy(CPUVirtualAddress, CpuData.get(), SizeInBytes);

	Context->CopyBufferRegion(SBTBuffer.GetResource(), 0, SBTUploadBuffer.GetResource(), 0, SizeInBytes);
}

D3D12_DISPATCH_RAYS_DESC D3D12RaytracingShaderBindingTable::GetDesc(
	UINT RayGenerationShaderIndex,
	UINT BaseMissShaderIndex) const
{
	UINT64 RayGenerationShaderTableSizeInBytes = RayGenerationShaderTable->GetSizeInBytes();
	UINT64 MissShaderTableSizeInBytes		   = MissShaderTable->GetSizeInBytes();
	UINT64 HitGroupShaderTableSizeInBytes	   = HitGroupShaderTable->GetSizeInBytes();

	UINT64 RayGenerationShaderRecordStride = RayGenerationShaderTable->GetStrideInBytes();
	UINT64 MissShaderRecordStride		   = MissShaderTable->GetStrideInBytes();
	UINT64 HitGroupShaderRecordStride	   = HitGroupShaderTable->GetStrideInBytes();

	D3D12_GPU_VIRTUAL_ADDRESS BaseAddress = SBTBuffer.GetGpuVirtualAddress();

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
