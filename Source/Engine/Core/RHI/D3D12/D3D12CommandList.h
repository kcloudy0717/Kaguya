#pragma once
#include "D3D12Common.h"

class D3D12Resource;
class D3D12CommandQueue;

class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12CommandAllocatorPool(
		D3D12LinkedDevice*		Parent,
		D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

	[[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator();

	void DiscardCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator,
		D3D12SyncHandle								   SyncHandle);

private:
	D3D12_COMMAND_LIST_TYPE									   CommandListType;
	CFencePool<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> CommandAllocatorPool;
};

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

	UINT Flush(
		ID3D12GraphicsCommandList* GraphicsCommandList);

	void Add(
		const D3D12_RESOURCE_BARRIER& ResourceBarrier);

	void AddTransition(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES StateBefore,
		D3D12_RESOURCE_STATES StateAfter,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AddAliasing(
		D3D12Resource* BeforeResource,
		D3D12Resource* AfterResource);

	void AddUAV(
		D3D12Resource* Resource);

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

class D3D12CommandListHandle : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandListHandle() noexcept = default;
	explicit D3D12CommandListHandle(
		D3D12LinkedDevice*		Parent,
		D3D12_COMMAND_LIST_TYPE Type);

	// Non-copyable but movable
	D3D12CommandListHandle(const D3D12CommandListHandle&) = delete;
	D3D12CommandListHandle& operator=(const D3D12CommandListHandle&) = delete;
	D3D12CommandListHandle(D3D12CommandListHandle&& D3D12CommandListHandle) noexcept;
	D3D12CommandListHandle& operator=(D3D12CommandListHandle&& D3D12CommandListHandle) noexcept;

	[[nodiscard]] ID3D12CommandList*		  GetCommandList() const noexcept { return GraphicsCommandList.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const noexcept { return GraphicsCommandList.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const noexcept { return GraphicsCommandList4.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const noexcept { return GraphicsCommandList6.Get(); }
	ID3D12GraphicsCommandList*				  operator->() const noexcept { return GetGraphicsCommandList(); }

	void Open(ID3D12CommandAllocator* CommandAllocator);
	void Close();

	void TransitionBarrier(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(
		D3D12Resource* BeforeResource,
		D3D12Resource* AfterResource);

	void UAVBarrier(
		D3D12Resource* Resource);

	void FlushResourceBarriers();

	bool AssertResourceState(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource);

private:
	// Resolve resource barriers that are needed before this command list is executed
	std::vector<D3D12_RESOURCE_BARRIER> ResolveResourceBarriers();

private:
	friend class D3D12CommandQueue;

	D3D12_COMMAND_LIST_TYPE Type;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>  GraphicsCommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GraphicsCommandList4;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> GraphicsCommandList6;
#ifdef D3D12_DEBUG_RESOURCE_STATES
	Microsoft::WRL::ComPtr<ID3D12DebugCommandList> DebugCommandList;
#endif
	D3D12ResourceStateTracker ResourceStateTracker;
	ResourceBarrierBatch	  ResourceBarrierBatch;
};
