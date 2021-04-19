#include "pch.h"
#include "ResourceStateTracker.h"

void ResourceStateTracker::AddResourceState(ID3D12Resource* pResource, D3D12_RESOURCE_STATES ResourceStates)
{
	if (pResource)
	{
		m_ResourceStates[pResource].SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, ResourceStates);
	}
}

bool ResourceStateTracker::RemoveResourceState(ID3D12Resource* pResource)
{
	if (pResource)
	{
		if (auto iter = m_ResourceStates.find(pResource);
			iter != m_ResourceStates.end())
		{
			m_ResourceStates.erase(iter);
			return true;
		}
	}

	return false;
}

void ResourceStateTracker::SetResourceState(ID3D12Resource* pResource, UINT Subresource, D3D12_RESOURCE_STATES ResourceStates)
{
	if (pResource)
	{
		m_ResourceStates[pResource].SetSubresourceState(Subresource, ResourceStates);
	}
}

void ResourceStateTracker::UpdateResourceStates(const ResourceStateTracker& ResourceStateTracker)
{
	for (const auto& resourceState : ResourceStateTracker.m_ResourceStates)
	{
		m_ResourceStates[resourceState.first] = resourceState.second;
	}
}

void ResourceStateTracker::Reset()
{
	m_ResourceStates.clear();
}

UINT ResourceStateTracker::FlushPendingResourceBarriers(const ResourceStateTracker& GlobalResourceStateTracker, ID3D12GraphicsCommandList* pCommandList)
{
	std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers;
	resourceBarriers.reserve(m_PendingResourceBarriers.size());

	for (auto pendingBarrier : m_PendingResourceBarriers)
	{
		assert(pendingBarrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);

		D3D12_RESOURCE_TRANSITION_BARRIER pendingTransition = pendingBarrier.Transition;
		auto resourceState = GlobalResourceStateTracker.Find(pendingTransition.pResource);
		if (resourceState.has_value())
		{
			// If all subresources are being transitioned, and there are multiple
			// subresources of the resource that are in a different state compared to
			// the global resource state then we need to transition all of them
			if (pendingTransition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
				!resourceState->AreAllSubresourcesSame())
			{
				for (auto subresourceState : *resourceState)
				{
					// If the subresource state is already the same as the transition state
					if (pendingTransition.StateAfter == subresourceState.second)
					{
						continue;
					}

					D3D12_RESOURCE_BARRIER newBarrier = pendingBarrier;
					newBarrier.Transition.Subresource = subresourceState.first;
					newBarrier.Transition.StateBefore = subresourceState.second;
					resourceBarriers.push_back(newBarrier);
				}
			}
			else
			{
				// We're only transitioning a particular subresource (could be all, or just 1)
				D3D12_RESOURCE_STATES state = resourceState->GetSubresourceState(pendingTransition.Subresource);
				if (pendingTransition.StateAfter == state)
				{
					continue;
				}

				pendingBarrier.Transition.StateBefore = state;
				resourceBarriers.push_back(pendingBarrier);
			}
		}
	}

	UINT numBarriers = resourceBarriers.size();
	if (numBarriers > 0)
	{
		pCommandList->ResourceBarrier(numBarriers, resourceBarriers.data());
	}
	m_PendingResourceBarriers.clear();
	return numBarriers;
}

UINT ResourceStateTracker::FlushResourceBarriers(ID3D12GraphicsCommandList* pCommandList)
{
	UINT numBarriers = m_ResourceBarriers.size();
	if (numBarriers > 0)
	{
		pCommandList->ResourceBarrier(numBarriers, m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}
	return numBarriers;
}

void ResourceStateTracker::ResourceBarrier(const D3D12_RESOURCE_BARRIER& Barrier)
{
	switch (Barrier.Type)
	{
	case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
	{
		// First check if there is already a known "final" state for the given resource.
		// If there is, the resource has been used on the command list before and
		// already has a known state within the command list execution.
		if (auto iter = m_ResourceStates.find(Barrier.Transition.pResource);
			iter != m_ResourceStates.end())
		{
			auto& resourceState = iter->second;
			// If the known final state of the resource is different...
			if (Barrier.Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES &&
				!resourceState.AreAllSubresourcesSame())
			{
				// First transition all of the subresources if they are different than the StateAfter.
				for (const auto& subresourceState : resourceState)
				{
					if (Barrier.Transition.StateAfter != subresourceState.second)
					{
						D3D12_RESOURCE_BARRIER newBarrier = Barrier;
						newBarrier.Transition.Subresource = subresourceState.first;
						newBarrier.Transition.StateBefore = subresourceState.second;
						m_ResourceBarriers.push_back(newBarrier);
					}
				}
			}
			else
			{
				auto finalState = resourceState.GetSubresourceState(Barrier.Transition.Subresource);
				if (Barrier.Transition.StateAfter != finalState)
				{
					// Push a new transition barrier with the correct before state.
					D3D12_RESOURCE_BARRIER newBarrier = Barrier;
					newBarrier.Transition.StateBefore = finalState;
					m_ResourceBarriers.push_back(newBarrier);
				}
			}
		}
		else // In this case, the resource is being used on the command list for the first time. 
		{
			// Add a pending barrier. The pending barriers will be resolved
			// before the command list is executed on the command queue.
			m_PendingResourceBarriers.push_back(Barrier);
		}

		// Push the final known state (possibly replacing the previously known state for the subresource).
		m_ResourceStates[Barrier.Transition.pResource].SetSubresourceState(Barrier.Transition.Subresource, Barrier.Transition.StateAfter);
	}
	break;

	case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
	case D3D12_RESOURCE_BARRIER_TYPE_UAV:
	{
		m_ResourceBarriers.push_back(Barrier);
	}
	break;
	}
}

std::optional<CResourceState> ResourceStateTracker::Find(ID3D12Resource* pResource) const
{
	if (auto iter = m_ResourceStates.find(pResource);
		iter != m_ResourceStates.end())
	{
		return iter->second;
	}

	return {};
}