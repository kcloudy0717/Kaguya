#include "RaytracingAccelerationStructure.h"
#include <RenderCore/RenderCore.h>

RaytracingAccelerationStructure::RaytracingAccelerationStructure(UINT NumHitGroups, size_t NumInstances)
	: NumHitGroups(NumHitGroups)
	, NumInstances(NumInstances)
	, TopLevelAccelerationStructure(NumInstances)
{
	StaticMeshes.reserve(NumInstances);

	Manager = D3D12RaytracingAccelerationStructureManager(RenderCore::Device->GetDevice(), 6_MiB);

	Null = D3D12ShaderResourceView(RenderCore::Device->GetDevice(), nullptr);
}

void RaytracingAccelerationStructure::Initialize()
{
	InstanceDescs = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * NumInstances,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	InstanceDescs.Initialize();
}

void RaytracingAccelerationStructure::Reset()
{
	TopLevelAccelerationStructure.Reset();
	StaticMeshes.clear();
	ReferencedGeometries.clear();
	CurrentInstanceID						   = 0;
	CurrentInstanceContributionToHitGroupIndex = 0;
}

void RaytracingAccelerationStructure::AddInstance(const Transform& Transform, StaticMeshComponent* StaticMesh)
{
	assert(TopLevelAccelerationStructure.size() < NumInstances);

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

void RaytracingAccelerationStructure::Build(D3D12CommandContext& Context)
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

	for (auto Geometry : ReferencedGeometries)
	{
		Geometry->AccelerationStructure = Manager.GetAccelerationStructureAddress(Geometry->BlasIndex);
	}

	D3D12ScopedEvent(Context, "TLAS");
	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		Instance.AccelerationStructure = StaticMeshes[i]->Mesh->AccelerationStructure;
	}

	UINT64 ScratchSize = 0, ResultSize = 0;
	TopLevelAccelerationStructure.ComputeMemoryRequirements(
		RenderCore::Device->GetD3D12Device5(),
		&ScratchSize,
		&ResultSize);

	if (!TlasScratch.GetResource() || TlasScratch.GetDesc().Width < ScratchSize)
	{
		// TLAS Scratch
		TlasScratch = D3D12Buffer(
			RenderCore::Device->GetDevice(),
			ScratchSize,
			0,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	if (!TlasResult.GetResource() || TlasResult.GetDesc().Width < ResultSize)
	{
		// TLAS Result
		TlasResult = D3D12ASBuffer(RenderCore::Device->GetDevice(), ResultSize);
		SRV		   = D3D12ShaderResourceView(RenderCore::Device->GetDevice(), &TlasResult);
	}

	// Create the description for each instance
	auto Instances = InstanceDescs.GetCpuVirtualAddress<D3D12_RAYTRACING_INSTANCE_DESC>();
	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		Instances[i] = Instance;
	}

	TopLevelAccelerationStructure.Generate(
		Context.GetGraphicsCommandList6(),
		TlasScratch.GetResource(),
		TlasResult.GetResource(),
		InstanceDescs.GetGpuVirtualAddress());
}

void RaytracingAccelerationStructure::PostBuild(D3D12SyncHandle SyncHandle)
{
	for (const auto& Geometry : ReferencedGeometries)
	{
		if (Geometry->BlasIndex != UINT64_MAX)
		{
			Manager.SetSyncHandle(Geometry->BlasIndex, SyncHandle);
		}
	}
}
