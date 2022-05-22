#include "RaytracingAccelerationStructure.h"

RaytracingAccelerationStructure::RaytracingAccelerationStructure(RHI::D3D12Device* Device, UINT NumHitGroups)
	: Device(Device)
	, NumHitGroups(NumHitGroups)
{
	Manager = RHI::D3D12RaytracingManager(Device->GetLinkedDevice(), 6 * 1024 * 1024);
	Null	= RHI::D3D12ShaderResourceView(Device->GetLinkedDevice(), nullptr);
}

void RaytracingAccelerationStructure::Reset()
{
	TopLevelAccelerationStructure.Reset();
	StaticMeshes.clear();
	ReferencedGeometries.clear();
	CurrentInstanceID						   = 0;
	CurrentInstanceContributionToHitGroupIndex = 0;
}

void RaytracingAccelerationStructure::AddInstance(const Math::Transform& Transform, StaticMeshComponent* StaticMesh)
{
	D3D12_RAYTRACING_INSTANCE_DESC RaytracingInstanceDesc = {};
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(RaytracingInstanceDesc.Transform), Transform.Matrix());
	RaytracingInstanceDesc.InstanceID						   = CurrentInstanceID++;
	RaytracingInstanceDesc.InstanceMask						   = RAYTRACING_INSTANCEMASK_ALL;
	RaytracingInstanceDesc.InstanceContributionToHitGroupIndex = CurrentInstanceContributionToHitGroupIndex;
	RaytracingInstanceDesc.Flags							   = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	RaytracingInstanceDesc.AccelerationStructure			   = NULL; // Resolved in Build

	TopLevelAccelerationStructure.AddInstance(RaytracingInstanceDesc);
	StaticMeshes.push_back(StaticMesh);
	ReferencedGeometries.insert(StaticMesh->Mesh);

	CurrentInstanceContributionToHitGroupIndex += StaticMesh->Mesh->Blas.Size() * NumHitGroups;
}

void RaytracingAccelerationStructure::Build(RHI::D3D12CommandContext& Context)
{
	bool AnyBlasBuild = false;

	// Build BLASes (If any)
	for (auto Geometry : ReferencedGeometries)
	{
		if (Geometry->BlasValid)
		{
			continue;
		}
		Geometry->BlasValid = true;
		AnyBlasBuild |= Geometry->BlasValid;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = Geometry->Blas.GetInputsDesc();
		Geometry->BlasIndex											= Manager.Build(Context.GetGraphicsCommandList4(), Inputs);
	}

	if (AnyBlasBuild)
	{
		Context.UAVBarrier(nullptr);
		Context.FlushResourceBarriers();
		Manager.Copy(Context.GetGraphicsCommandList4());
	}

	{
		D3D12ScopedEvent(Context, "BLAS Compact");
		for (auto Geometry : ReferencedGeometries)
		{
			Manager.Compact(Context.GetGraphicsCommandList4(), Geometry->BlasIndex);
		}
	}

	// Update acceleration structure address (in the case if it got compacted)
	for (auto Geometry : ReferencedGeometries)
	{
		Geometry->AccelerationStructure = Manager.GetAddress(Geometry->BlasIndex);
	}

	D3D12ScopedEvent(Context, "TLAS");
	size_t i = 0;
	for (auto& Instance : TopLevelAccelerationStructure)
	{
		Instance.AccelerationStructure = StaticMeshes[i++]->Mesh->AccelerationStructure;
	}

	// Compute memory requirements
	UINT64 ScratchSize = 0, ResultSize = 0;
	UINT64 InstanceSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * TopLevelAccelerationStructure.size();
	TopLevelAccelerationStructure.ComputeMemoryRequirements(Device->GetD3D12Device5(), &ScratchSize, &ResultSize);

	// Create scratch, result, and instance buffers
	if (!TlasScratch || TlasScratch.GetDesc().Width < ScratchSize)
	{
		TlasScratch = RHI::D3D12Buffer(Device->GetLinkedDevice(), ScratchSize, 0, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	if (!TlasResult || TlasResult.GetDesc().Width < ResultSize)
	{
		TlasResult = RHI::D3D12ASBuffer(Device->GetLinkedDevice(), ResultSize);
		SRV		   = RHI::D3D12ShaderResourceView(Device->GetLinkedDevice(), &TlasResult);
	}

	if (!InstanceDescsBuffer || InstanceDescsBuffer.GetDesc().Width < InstanceSize)
	{
		InstanceDescsBuffer = RHI::D3D12Buffer(Device->GetLinkedDevice(), InstanceSize, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
	}

	// Create the description for each instance
	auto Instances = InstanceDescsBuffer.GetCpuVirtualAddress<D3D12_RAYTRACING_INSTANCE_DESC>();
	i			   = 0;
	for (auto& Instance : TopLevelAccelerationStructure)
	{
		Instances[i++] = Instance;
	}

	TopLevelAccelerationStructure.Generate(
		Context.GetGraphicsCommandList6(),
		TlasScratch.GetResource(),
		TlasResult.GetResource(),
		InstanceDescsBuffer.GetGpuVirtualAddress());
}

void RaytracingAccelerationStructure::PostBuild(RHI::D3D12SyncHandle SyncHandle)
{
	for (const auto& Geometry : ReferencedGeometries)
	{
		if (Geometry->BlasIndex != UINT64_MAX)
		{
			Manager.SetSyncHandle(Geometry->BlasIndex, SyncHandle);
		}
	}
}
