#include "pch.h"
#include "ResourceStateTracker.h"

std::vector<PendingResourceBarrier>& ResourceStateTracker::GetPendingResourceBarriers()
{
	return PendingResourceBarriers;
}

CResourceState& ResourceStateTracker::GetResourceState(Resource* Resource)
{
	CResourceState& ResourceState = ResourceStates[Resource];
	ConditionalInitialize(ResourceState);
	return ResourceState;
}

void ResourceStateTracker::Reset()
{
	ResourceStates.clear();
	PendingResourceBarriers.clear();
}
