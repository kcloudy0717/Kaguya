#include "pch.h"
#include "ResourceStateTracker.h"

std::vector<PendingResourceBarrier>& ResourceStateTracker::GetPendingResourceBarriers()
{
	return m_PendingResourceBarriers;
}

void ResourceStateTracker::Reset()
{
	m_ResourceStates.clear();
	m_PendingResourceBarriers.clear();
}
