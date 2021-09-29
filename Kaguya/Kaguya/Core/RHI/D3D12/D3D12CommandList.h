#pragma once
#include "D3D12Common.h"
#include "D3D12ResourceStateTracker.h"

class D3D12CommandQueue;

class D3D12CommandAllocator
{
public:
	explicit D3D12CommandAllocator(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type);

	operator ID3D12CommandAllocator*() const noexcept { return CommandAllocator.Get(); }

	[[nodiscard]] bool IsReady() const { return CommandSyncPoint.IsComplete(); }

	[[nodiscard]] D3D12CommandSyncPoint GetSyncPoint() const { return CommandSyncPoint; }

	[[nodiscard]] bool HasValidSyncPoint() const { return CommandSyncPoint.IsValid(); }

	void SetSyncPoint(const D3D12CommandSyncPoint& SyncPoint)
	{
		assert(SyncPoint.IsValid());
		CommandSyncPoint = SyncPoint;
	}

	void Reset()
	{
		assert(IsReady());
		VERIFY_D3D12_API(CommandAllocator->Reset());
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
	D3D12CommandSyncPoint						   CommandSyncPoint;
};

class D3D12CommandListHandle : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandListHandle() noexcept = default;
	explicit D3D12CommandListHandle(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE Type);
	~D3D12CommandListHandle();

	// Non-copyable but movable
	D3D12CommandListHandle(const D3D12CommandListHandle&) = delete;
	D3D12CommandListHandle& operator=(const D3D12CommandListHandle&) = delete;
	D3D12CommandListHandle(D3D12CommandListHandle&& D3D12CommandListHandle) noexcept;
	D3D12CommandListHandle& operator=(D3D12CommandListHandle&& D3D12CommandListHandle) noexcept;

	[[nodiscard]] ID3D12CommandList*		  GetCommandList() const { return GraphicsCommandList.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const { return GraphicsCommandList.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const { return GraphicsCommandList4.Get(); }
	[[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const { return GraphicsCommandList6.Get(); }
#ifdef NVIDIA_NSIGHT_AFTERMATH
	[[nodiscard]] GFSDK_Aftermath_ContextHandle GetAftermathContextHandle() const { return AftermathContextHandle; }
#endif

	ID3D12GraphicsCommandList* operator->() const { return GetGraphicsCommandList(); }

	void Close()
	{
		FlushResourceBarriers();
		VERIFY_D3D12_API(GraphicsCommandList->Close());
	}

	void Reset(D3D12CommandAllocator* CommandAllocator)
	{
		this->CommandAllocator = CommandAllocator;

		VERIFY_D3D12_API(GraphicsCommandList->Reset(*CommandAllocator, nullptr));

		// Reset resource state tracking and resource barriers
		ResourceStateTracker.Reset();
		ResourceBarrierBatch.Reset();
	}

	void SetSyncPoint(const D3D12CommandSyncPoint& SyncPoint) { CommandAllocator->SetSyncPoint(SyncPoint); }

	CResourceState& GetResourceState(D3D12Resource* Resource)
	{
		return ResourceStateTracker.GetResourceState(Resource);
	}

	std::vector<D3D12_RESOURCE_BARRIER> ResolveResourceBarriers();

	void TransitionBarrier(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource);

	void UAVBarrier(D3D12Resource* Resource);

	void FlushResourceBarriers();

	bool AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource);

private:
	D3D12_COMMAND_LIST_TYPE Type;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>  GraphicsCommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GraphicsCommandList4;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> GraphicsCommandList6;

#ifdef D3D12_DEBUG_RESOURCE_STATES
	Microsoft::WRL::ComPtr<ID3D12DebugCommandList> DebugCommandList;
#endif
#ifdef NVIDIA_NSIGHT_AFTERMATH
	GFSDK_Aftermath_ContextHandle AftermathContextHandle = nullptr;
#endif

	D3D12CommandAllocator*	  CommandAllocator;
	D3D12ResourceStateTracker ResourceStateTracker;
	ResourceBarrierBatch	  ResourceBarrierBatch;
};
