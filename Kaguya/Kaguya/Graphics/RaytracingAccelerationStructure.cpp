#include "RaytracingAccelerationStructure.h"
#include <RenderCore/RenderCore.h>

RaytracingAccelerationStructure::RaytracingAccelerationStructure(UINT NumHitGroups, size_t NumInstances)
	: NumHitGroups(NumHitGroups)
	, NumInstances(NumInstances)
{
	MeshRenderers.reserve(NumInstances);
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
	MeshRenderers.clear();
	ReferencedGeometries.clear();
	InstanceContributionToHitGroupIndex = 0;
}

void RaytracingAccelerationStructure::AddInstance(const Transform& Transform, MeshRenderer* MeshRenderer)
{
	D3D12_RAYTRACING_INSTANCE_DESC RaytracingInstanceDesc = {};
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(RaytracingInstanceDesc.Transform), Transform.Matrix());
	RaytracingInstanceDesc.InstanceID						   = TopLevelAccelerationStructure.Size();
	RaytracingInstanceDesc.InstanceMask						   = RAYTRACING_INSTANCEMASK_ALL;
	RaytracingInstanceDesc.InstanceContributionToHitGroupIndex = InstanceContributionToHitGroupIndex;
	RaytracingInstanceDesc.Flags							   = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	RaytracingInstanceDesc.AccelerationStructure			   = NULL; // Resolved later

	TopLevelAccelerationStructure.AddInstance(RaytracingInstanceDesc);
	MeshRenderers.push_back(MeshRenderer);
	ReferencedGeometries.insert(MeshRenderer->pMeshFilter->Mesh);

	InstanceContributionToHitGroupIndex += MeshRenderer->pMeshFilter->Mesh->Blas.Size() * NumHitGroups;
}

void RaytracingAccelerationStructure::Build(D3D12CommandContext& Context)
{
	PIXScopedEvent(Context.CommandListHandle.GetGraphicsCommandList(), 0, L"TLAS");

	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		Instance.AccelerationStructure = MeshRenderers[i]->pMeshFilter->Mesh->AccelerationStructure;
	}

	UINT64 ScratchSize = 0, ResultSize = 0;
	TopLevelAccelerationStructure.ComputeMemoryRequirements(
		RenderCore::Device->GetD3D12Device5(),
		&ScratchSize,
		&ResultSize);

	if (!TlasScratch || TlasScratch.GetDesc().Width < ScratchSize)
	{
		// TLAS Scratch
		TlasScratch = D3D12Buffer(
			RenderCore::Device->GetDevice(),
			ScratchSize,
			0,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	if (!TlasResult || TlasResult.GetDesc().Width < ResultSize)
	{
		// TLAS Result
		TlasResult = D3D12ASBuffer(RenderCore::Device->GetDevice(), ResultSize);
	}

	// Create the description for each instance
	auto Instances = InstanceDescs.GetCpuVirtualAddress<D3D12_RAYTRACING_INSTANCE_DESC>();
	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		Instances[i] = Instance;
	}

	TopLevelAccelerationStructure.Generate(
		Context.CommandListHandle.GetGraphicsCommandList6(),
		TlasScratch,
		TlasResult,
		InstanceDescs.GetGpuVirtualAddress());
}
