#include "ResourceStateTracker.h"

std::vector<PendingResourceBarrier>& ResourceStateTracker::GetPendingResourceBarriers()
{
	return PendingResourceBarriers;
}

CResourceState& ResourceStateTracker::GetResourceState(Resource* Resource)
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

void ResourceStateTracker::Reset()
{
	ResourceStates.clear();
	PendingResourceBarriers.clear();
}
