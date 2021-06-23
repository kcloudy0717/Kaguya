#include "pch.h"
#include "RaytracingAccelerationStructure.h"

#include "RenderDevice.h"

RaytracingAccelerationStructure::RaytracingAccelerationStructure(UINT NumHitGroups)
{
	auto& RenderDevice = RenderDevice::Instance();

	m_NumHitGroups = NumHitGroups;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType					= D3D12_HEAP_TYPE_UPLOAD;
	m_InstanceDescs							= RenderDevice.CreateBuffer(
		&allocationDesc,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * Scene::MAX_INSTANCE_SUPPORTED,
		D3D12_RESOURCE_FLAG_NONE,
		0,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	ThrowIfFailed(m_InstanceDescs->pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pInstanceDescs)));
}

void RaytracingAccelerationStructure::AddInstance(MeshRenderer* pMeshRenderer)
{
	auto& AccelerationStructure = pMeshRenderer->pMeshFilter->Mesh->AccelerationStructure;

	Entity entity(pMeshRenderer->Handle, pMeshRenderer->pScene);

	auto& transform = entity.GetComponent<Transform>();

	D3D12_RAYTRACING_INSTANCE_DESC dxrInstanceDesc = {};
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(dxrInstanceDesc.Transform), transform.Matrix());
	dxrInstanceDesc.InstanceID							= m_TopLevelAccelerationStructure.size();
	dxrInstanceDesc.InstanceMask						= RAYTRACING_INSTANCEMASK_ALL;
	dxrInstanceDesc.InstanceContributionToHitGroupIndex = m_InstanceContributionToHitGroupIndex;
	dxrInstanceDesc.Flags								= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	dxrInstanceDesc.AccelerationStructure				= AccelerationStructure->pResource->GetGPUVirtualAddress();

	m_TopLevelAccelerationStructure.AddInstance(dxrInstanceDesc);
	m_MeshRenderers.push_back(pMeshRenderer);

	m_InstanceContributionToHitGroupIndex += pMeshRenderer->pMeshFilter->Mesh->BLAS.size() * m_NumHitGroups;
}

void RaytracingAccelerationStructure::Build(CommandList& CommandList)
{
	auto& RenderDevice = RenderDevice::Instance();

	PIXScopedEvent(CommandList.GetCommandList(), 0, L"Top Level Acceleration Structure Generation");

	UINT64 scratchSize, resultSize;
	m_TopLevelAccelerationStructure.ComputeMemoryRequirements(RenderDevice.GetDevice(), &scratchSize, &resultSize);

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags					= D3D12MA::ALLOCATION_FLAG_COMMITTED;
	allocationDesc.HeapType					= D3D12_HEAP_TYPE_DEFAULT;

	if (!m_TLASScratch || m_TLASScratch->pResource->GetDesc().Width < scratchSize)
	{
		// TLAS Scratch
		m_TLASScratch = RenderDevice.CreateBuffer(
			&allocationDesc,
			scratchSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			0,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	if (!m_TLASResult || m_TLASResult->pResource->GetDesc().Width < resultSize)
	{
		// TLAS Result
		m_TLASResult = RenderDevice.CreateBuffer(
			&allocationDesc,
			resultSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			0,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	}

	// Create the description for each instance
	for (auto [i, instance] : enumerate(m_TopLevelAccelerationStructure))
	{
		m_pInstanceDescs[i] = instance;
	}

	m_TopLevelAccelerationStructure.Generate(
		CommandList,
		m_TLASScratch->pResource.Get(),
		m_TLASResult->pResource.Get(),
		m_InstanceDescs->pResource->GetGPUVirtualAddress());
}
