#pragma once
#include <d3d12.h>
#include <unordered_map>
#include "D3D12Resource.h"

// https://www.youtube.com/watch?v=nmB2XMasz2o, Resource state tracking
struct PendingResourceBarrier
{
	D3D12Resource*		  Resource;
	D3D12_RESOURCE_STATES State;
	UINT				  Subresource;
};

struct ResourceBarrierBatch
{
	enum
	{
		BatchCount = 64
	};

	ResourceBarrierBatch()
	{
		std::memset(ResourceBarriers, NULL, sizeof(ResourceBarriers));
		NumResourceBarriers = 0;
	}

	void Reset() { NumResourceBarriers = 0; }

	UINT Flush(ID3D12GraphicsCommandList* GraphicsCommandList)
	{
		if (NumResourceBarriers > 0)
		{
			GraphicsCommandList->ResourceBarrier(NumResourceBarriers, ResourceBarriers);
			Reset();
		}
		return NumResourceBarriers;
	}

	void Add(const D3D12_RESOURCE_BARRIER& ResourceBarrier)
	{
		assert(NumResourceBarriers < BatchCount);
		ResourceBarriers[NumResourceBarriers++] = ResourceBarrier;
	}

	void AddTransition(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES StateBefore,
		D3D12_RESOURCE_STATES StateAfter,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		Add(CD3DX12_RESOURCE_BARRIER::Transition(Resource->GetResource(), StateBefore, StateAfter, Subresource));
	}

	void AddAliasing(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
	{
		Add(CD3DX12_RESOURCE_BARRIER::Aliasing(BeforeResource->GetResource(), AfterResource->GetResource()));
	}

	void AddUAV(D3D12Resource* Resource)
	{
		Add(CD3DX12_RESOURCE_BARRIER::UAV(Resource ? Resource->GetResource() : nullptr));
	}

	D3D12_RESOURCE_BARRIER ResourceBarriers[BatchCount];
	UINT				   NumResourceBarriers;
};

class D3D12ResourceStateTracker
{
public:
	std::vector<PendingResourceBarrier>& GetPendingResourceBarriers();

	CResourceState& GetResourceState(D3D12Resource* Resource);

	void Reset();

	void Add(const PendingResourceBarrier& PendingResourceBarrier)
	{
		PendingResourceBarriers.push_back(PendingResourceBarrier);
	}

private:
	std::unordered_map<D3D12Resource*, CResourceState> ResourceStates;

	// Pending resource transitions are committed to a separate commandlist before this commandlist
	// is executed on the command queue. This guarantees that resources will
	// be in the expected state at the beginning of a command list.
	std::vector<PendingResourceBarrier> PendingResourceBarriers;
};
