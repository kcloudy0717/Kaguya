#include "RaytracingAccelerationStructure.h"
#include <RenderCore/RenderCore.h>

RaytracingAccelerationStructure::RaytracingAccelerationStructure(UINT NumHitGroups)
	: NumHitGroups(NumHitGroups)
{
}

void RaytracingAccelerationStructure::Initialize()
{
	InstanceDescs = D3D12Buffer(
		RenderCore::pAdapter->GetDevice(),
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * World::InstanceLimit,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	InstanceDescs.Initialize();

	pInstanceDescs = InstanceDescs.GetCPUVirtualAddress<D3D12_RAYTRACING_INSTANCE_DESC>();
}

void RaytracingAccelerationStructure::AddInstance(const Transform& Transform, MeshRenderer* pMeshRenderer)
{
	D3D12_RAYTRACING_INSTANCE_DESC RaytracingInstanceDesc = {};
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(RaytracingInstanceDesc.Transform), Transform.Matrix());
	RaytracingInstanceDesc.InstanceID						   = TopLevelAccelerationStructure.size();
	RaytracingInstanceDesc.InstanceMask						   = RAYTRACING_INSTANCEMASK_ALL;
	RaytracingInstanceDesc.InstanceContributionToHitGroupIndex = InstanceContributionToHitGroupIndex;
	RaytracingInstanceDesc.Flags							   = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	RaytracingInstanceDesc.AccelerationStructure			   = NULL; // Resolved later

	TopLevelAccelerationStructure.AddInstance(RaytracingInstanceDesc);
	MeshRenderers.push_back(pMeshRenderer);
	ReferencedGeometries.insert(pMeshRenderer->pMeshFilter->Mesh);

	InstanceContributionToHitGroupIndex +=
		static_cast<UINT>(pMeshRenderer->pMeshFilter->Mesh->BLAS.size()) * NumHitGroups;
}

void RaytracingAccelerationStructure::Build(D3D12CommandContext& Context)
{
	PIXScopedEvent(Context.CommandListHandle.GetGraphicsCommandList(), 0, L"TLAS");

	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		MeshRenderer* pMeshRenderer = MeshRenderers[i];

		Instance.AccelerationStructure = pMeshRenderer->pMeshFilter->Mesh->AccelerationStructure;
	}

	UINT64 ScratchSize, ResultSize;
	TopLevelAccelerationStructure.ComputeMemoryRequirements(
		RenderCore::pAdapter->GetD3D12Device5(),
		&ScratchSize,
		&ResultSize);

	if (!TLASScratch || TLASScratch.GetDesc().Width < ScratchSize)
	{
		// TLAS Scratch
		TLASScratch = D3D12Buffer(
			RenderCore::pAdapter->GetDevice(),
			ScratchSize,
			0,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	if (!TLASResult || TLASResult.GetDesc().Width < ResultSize)
	{
		// TLAS Result
		TLASResult = D3D12ASBuffer(RenderCore::pAdapter->GetDevice(), ResultSize);
	}

	// Create the description for each instance
	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		pInstanceDescs[i] = Instance;
	}

	TopLevelAccelerationStructure.Generate(
		Context.CommandListHandle.GetGraphicsCommandList6(),
		TLASScratch,
		TLASResult,
		InstanceDescs.GetGPUVirtualAddress());
}
