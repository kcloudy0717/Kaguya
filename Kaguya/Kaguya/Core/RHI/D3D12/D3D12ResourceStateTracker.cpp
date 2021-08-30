#include "D3D12ResourceStateTracker.h"

std::vector<PendingResourceBarrier>& D3D12ResourceStateTracker::GetPendingResourceBarriers()
{
	return PendingResourceBarriers;
}

CResourceState& D3D12ResourceStateTracker::GetResourceState(D3D12Resource* Resource)
{
	CResourceState& ResourceState = ResourceStates[Resource];
	// If ResourceState was just created, its state is uninitialized
	if (ResourceState.IsResourceStateUninitialized())
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
