#include "pch.h"
#include "RaytracingAccelerationStructure.h"

#include "RenderDevice.h"

RaytracingAccelerationStructure::RaytracingAccelerationStructure(UINT NumHitGroups)
	: NumHitGroups(NumHitGroups)
{
	auto& RenderDevice = RenderDevice::Instance();

	InstanceDescs = Buffer(
		RenderDevice.GetDevice(),
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * Scene::MAX_INSTANCE_SUPPORTED,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);

	pInstanceDescs = InstanceDescs.GetCPUVirtualAddress<D3D12_RAYTRACING_INSTANCE_DESC>();
}

void RaytracingAccelerationStructure::AddInstance(const Transform& Transform, MeshRenderer* pMeshRenderer)
{
	D3D12_RAYTRACING_INSTANCE_DESC dxrInstanceDesc = {};
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(dxrInstanceDesc.Transform), Transform.Matrix());
	dxrInstanceDesc.InstanceID							= TopLevelAccelerationStructure.size();
	dxrInstanceDesc.InstanceMask						= RAYTRACING_INSTANCEMASK_ALL;
	dxrInstanceDesc.InstanceContributionToHitGroupIndex = InstanceContributionToHitGroupIndex;
	dxrInstanceDesc.Flags								= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	dxrInstanceDesc.AccelerationStructure				= NULL; // Resolved later

	TopLevelAccelerationStructure.AddInstance(dxrInstanceDesc);
	MeshRenderers.push_back(pMeshRenderer);

	InstanceContributionToHitGroupIndex += pMeshRenderer->pMeshFilter->Mesh->BLAS.size() * NumHitGroups;
}

void RaytracingAccelerationStructure::Build(CommandContext& Context)
{
	auto& RenderDevice = RenderDevice::Instance();

	PIXScopedEvent(Context.CommandListHandle.GetGraphicsCommandList(), 0, L"TLAS");

	for (auto [i, Instance] : enumerate(TopLevelAccelerationStructure))
	{
		MeshRenderer* pMeshRenderer = MeshRenderers[i];

		const ASBuffer& AccelerationStructure = pMeshRenderer->pMeshFilter->Mesh->AccelerationStructure;

		Instance.AccelerationStructure = AccelerationStructure.GetGPUVirtualAddress();
	}

	UINT64 scratchSize, resultSize;
	TopLevelAccelerationStructure.ComputeMemoryRequirements(
		RenderDevice.GetDevice()->GetDevice5(),
		&scratchSize,
		&resultSize);

	if (!TLASScratch || TLASScratch.GetDesc().Width < scratchSize)
	{
		// TLAS Scratch
		TLASScratch = Buffer(
			RenderDevice.GetDevice(),
			scratchSize,
			0,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	if (!TLASResult || TLASResult.GetDesc().Width < resultSize)
	{
		// TLAS Result
		TLASResult = Buffer(
			RenderDevice.GetDevice(),
			resultSize,
			0,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	// Create the description for each instance
	for (auto [i, instance] : enumerate(TopLevelAccelerationStructure))
	{
		pInstanceDescs[i] = instance;
	}

	TopLevelAccelerationStructure.Generate(
		Context.CommandListHandle.GetGraphicsCommandList6(),
		TLASScratch,
		TLASResult,
		InstanceDescs.GetGPUVirtualAddress());
}
