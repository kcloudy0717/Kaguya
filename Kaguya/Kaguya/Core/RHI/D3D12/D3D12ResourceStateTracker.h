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
	static constexpr UINT NumBatches = 64;

	void Reset();

	UINT Flush(ID3D12GraphicsCommandList* GraphicsCommandList);

	void Add(const D3D12_RESOURCE_BARRIER& ResourceBarrier);
	void AddTransition(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES StateBefore,
		D3D12_RESOURCE_STATES StateAfter,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void AddAliasing(D3D12Resource* BeforeResource, D3D12Resource* AfterResource);
	void AddUAV(D3D12Resource* Resource);

	D3D12_RESOURCE_BARRIER ResourceBarriers[NumBatches] = {};
	UINT				   NumResourceBarriers			= 0;
};

class D3D12ResourceStateTracker
{
public:
	D3D12ResourceStateTracker() noexcept = default;

	std::vector<PendingResourceBarrier>& GetPendingResourceBarriers();

	CResourceState& GetResourceState(D3D12Resource* Resource);

	void Reset();

	void Add(const PendingResourceBarrier& PendingResourceBarrier);

private:
	std::unordered_map<D3D12Resource*, CResourceState> ResourceStates;

	// Pending resource transitions are committed to a separate commandlist before this commandlist
	// is executed on the command queue. This guarantees that resources will
	// be in the expected state at the beginning of a command list.
	std::vector<PendingResourceBarrier> PendingResourceBarriers;
};
