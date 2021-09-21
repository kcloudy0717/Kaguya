#include "D3D12ResourceStateTracker.h"

void ResourceBarrierBatch::Reset()
{
	NumResourceBarriers = 0;
}

UINT ResourceBarrierBatch::Flush(ID3D12GraphicsCommandList* GraphicsCommandList)
{
	if (NumResourceBarriers > 0)
	{
		GraphicsCommandList->ResourceBarrier(NumResourceBarriers, ResourceBarriers);
		Reset();
	}
	return NumResourceBarriers;
}

void ResourceBarrierBatch::Add(const D3D12_RESOURCE_BARRIER& ResourceBarrier)
{
	assert(NumResourceBarriers < NumBatches);
	ResourceBarriers[NumResourceBarriers++] = ResourceBarrier;
}

void ResourceBarrierBatch::AddTransition(
	D3D12Resource*		  Resource,
	D3D12_RESOURCE_STATES StateBefore,
	D3D12_RESOURCE_STATES StateAfter,
	UINT				  Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	Add(CD3DX12_RESOURCE_BARRIER::Transition(Resource->GetResource(), StateBefore, StateAfter, Subresource));
}

void ResourceBarrierBatch::AddAliasing(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
{
	Add(CD3DX12_RESOURCE_BARRIER::Aliasing(BeforeResource->GetResource(), AfterResource->GetResource()));
}

void ResourceBarrierBatch::AddUAV(D3D12Resource* Resource)
{
	Add(CD3DX12_RESOURCE_BARRIER::UAV(Resource ? Resource->GetResource() : nullptr));
}

std::vector<PendingResourceBarrier>& D3D12ResourceStateTracker::GetPendingResourceBarriers()
{
	return PendingResourceBarriers;
}

CResourceState& D3D12ResourceStateTracker::GetResourceState(D3D12Resource* Resource)
{
	CResourceState& ResourceState = ResourceStates[Resource];
	// If ResourceState was just created, its state is uninitialized
	if (ResourceState.IsUninitialized())
	{
		ResourceState = CResourceState(Resource->GetNumSubresources());
		ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNKNOWN);
	}
	return ResourceState;
}

void D3D12ResourceStateTracker::Reset()
{
	ResourceStates.clear();
	PendingResourceBarriers.clear();
}

void D3D12ResourceStateTracker::Add(const PendingResourceBarrier& PendingResourceBarrier)
{
	PendingResourceBarriers.push_back(PendingResourceBarrier);
}
