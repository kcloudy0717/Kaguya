#pragma once
#include <d3d12.h>
#include <unordered_map>
#include <optional>

// https://www.youtube.com/watch?v=nmB2XMasz2o&t=1036s, Resource state tracking

// Custom resource states
#define D3D12_RESOURCE_STATE_UNKNOWN (static_cast<D3D12_RESOURCE_STATES>(-1))

class CResourceState
{
public:
	auto begin()
	{
		return m_SubresourceState.begin();
	}

	auto end()
	{
		return m_SubresourceState.end();
	}

	// Returns true if all subresources have the same state
	bool AreAllSubresourcesSame() const noexcept
	{
		// Since we clear all subresource state if SetSubresourceState is set
		// to D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, we only need to check if the
		// data structure is empty;
		return m_SubresourceState.empty();
	}

	D3D12_RESOURCE_STATES GetSubresourceState(
		_In_ UINT Subresource) const noexcept
	{
		if (const auto iter = m_SubresourceState.find(Subresource);
			iter != m_SubresourceState.end())
		{
			return iter->second;
		}

		return m_State;
	}

	void SetSubresourceState(
		_In_ UINT Subresource,
		_In_ D3D12_RESOURCE_STATES State) noexcept
	{
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			m_State = State;
			m_SubresourceState.clear();
		}
		else
		{
			m_SubresourceState[Subresource] = State;
		}
	}

private:
	D3D12_RESOURCE_STATES m_State;
	std::unordered_map<UINT, D3D12_RESOURCE_STATES> m_SubresourceState;
};

class ResourceStateTracker
{
public:
	void AddResourceState(
		_In_ ID3D12Resource* pResource,
		_In_ D3D12_RESOURCE_STATES ResourceStates);

	bool RemoveResourceState(
		_In_ ID3D12Resource* pResource);

	void SetResourceState(
		_In_ ID3D12Resource* pResource,
		_In_ UINT Subresource,
		_In_ D3D12_RESOURCE_STATES ResourceStates);

	void UpdateResourceStates(
		_In_ const ResourceStateTracker& ResourceStateTracker);

	void Reset();

	UINT FlushPendingResourceBarriers(
		_In_ const ResourceStateTracker& GlobalResourceStateTracker,
		_In_ ID3D12GraphicsCommandList* pCommandList);

	UINT FlushResourceBarriers(
		_In_ ID3D12GraphicsCommandList* pCommandList);

	void ResourceBarrier(
		_In_ const D3D12_RESOURCE_BARRIER& Barrier);

	std::optional<CResourceState> Find(
		_In_ ID3D12Resource* pResource) const;

private:
	std::unordered_map<ID3D12Resource*, CResourceState> m_ResourceStates;

	// Pending resource transitions are committed to a separate commandlist before this commandlist
	// is executed on the command queue. This guarantees that resources will
	// be in the expected state at the beginning of a command list.
	std::vector<D3D12_RESOURCE_BARRIER>					m_PendingResourceBarriers;
	// Resource barriers that need to be committed to the command list.
	std::vector<D3D12_RESOURCE_BARRIER>					m_ResourceBarriers;
};
