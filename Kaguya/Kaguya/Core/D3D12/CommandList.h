#pragma once
#include "D3D12Common.h"
#include "ResourceStateTracker.h"
#include "RootSignature.h"
#include "PipelineState.h"

class CommandQueue;

class CommandAllocator
{
public:
	CommandAllocator(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE Type);

	operator ID3D12CommandAllocator*() { return pCommandAllocator.Get(); }

	bool IsReady() const { return CommandSyncPoint.IsComplete(); }

	CommandSyncPoint GetSyncPoint() const { return CommandSyncPoint; }

	bool HasValidSyncPoint() const { return CommandSyncPoint.IsValid(); }

	void SetSyncPoint(const CommandSyncPoint& SyncPoint)
	{
		assert(SyncPoint.IsValid());
		CommandSyncPoint = SyncPoint;
	}

	void Reset()
	{
		assert(IsReady());
		ASSERTD3D12APISUCCEEDED(pCommandAllocator->Reset());
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
	CommandSyncPoint							   CommandSyncPoint;
};

class CommandListHandle
{
public:
	CommandListHandle() noexcept = default;

	CommandListHandle(Device* Device, D3D12_COMMAND_LIST_TYPE Type, CommandQueue* CommandQueue);

	ID3D12CommandList*			GetCommandList() const { return pCommandList->GraphicsCommandList.Get(); }
	ID3D12GraphicsCommandList*	GetGraphicsCommandList() const { return pCommandList->GraphicsCommandList.Get(); }
	ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const { return pCommandList->GraphicsCommandList4.Get(); }
	ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const { return pCommandList->GraphicsCommandList6.Get(); }

#ifdef D3D12_NSIGHT_AFTERMATH
	GFSDK_Aftermath_ContextHandle GetAftermathContextHandle() const { return pCommandList->AftermathContextHandle; }
#endif

	ID3D12GraphicsCommandList* operator->() const { return GetGraphicsCommandList(); }

	operator bool() const { return pCommandList != nullptr; }

	void Close() { pCommandList->Close(); }

	void Reset(CommandAllocator* CommandAllocator) { pCommandList->Reset(CommandAllocator); }

	void SetSyncPoint(const CommandSyncPoint& SyncPoint) { pCommandList->pCommandAllocator->SetSyncPoint(SyncPoint); }

	std::vector<PendingResourceBarrier>& GetPendingResourceBarriers()
	{
		return pCommandList->ResourceStateTracker.GetPendingResourceBarriers();
	}

	CResourceState& GetResourceState(Resource* Resource)
	{
		return pCommandList->ResourceStateTracker.GetResourceState(Resource);
	}

	void TransitionBarrier(
		Resource*			  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(Resource* BeforeResource, Resource* AfterResource);

	void UAVBarrier(Resource* Resource);

	void FlushResourceBarriers();

	bool AssertResourceState(Resource* pResource, D3D12_RESOURCE_STATES State, UINT Subresource);

private:
	class CommandList : public DeviceChild
	{
	public:
		CommandList(Device* Device, D3D12_COMMAND_LIST_TYPE Type);
		~CommandList();

		void Close();

		void Reset(CommandAllocator* CommandAllocator);

		void FlushResourceBarriers();

		const D3D12_COMMAND_LIST_TYPE Type;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>  GraphicsCommandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GraphicsCommandList4;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> GraphicsCommandList6;

#ifdef D3D12_DEBUG_RESOURCE_STATES
		Microsoft::WRL::ComPtr<ID3D12DebugCommandList> DebugCommandList;
#endif
#ifdef D3D12_NSIGHT_AFTERMATH
		GFSDK_Aftermath_ContextHandle AftermathContextHandle = nullptr;
#endif

		CommandAllocator* pCommandAllocator;
		bool			  IsClosed;

		ResourceStateTracker ResourceStateTracker;
		ResourceBarrierBatch ResourceBarrierBatch;
	};

private:
	std::shared_ptr<CommandList> pCommandList;
};
