#pragma once
#include "D3D12Common.h"
#include "D3D12ResourceStateTracker.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"

class D3D12CommandQueue;

class D3D12CommandAllocator
{
public:
	D3D12CommandAllocator() noexcept = default;
	D3D12CommandAllocator(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type);

	operator ID3D12CommandAllocator*() { return CommandAllocator.Get(); }

	bool IsReady() const { return CommandSyncPoint.IsComplete(); }

	D3D12CommandSyncPoint GetSyncPoint() const { return CommandSyncPoint; }

	bool HasValidSyncPoint() const { return CommandSyncPoint.IsValid(); }

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

class D3D12CommandListHandle
{
public:
	D3D12CommandListHandle() noexcept = default;
	D3D12CommandListHandle(D3D12LinkedDevice* Device, D3D12_COMMAND_LIST_TYPE Type, D3D12CommandQueue* CommandQueue);

	ID3D12CommandList*			GetCommandList() const { return CommandList->GraphicsCommandList.Get(); }
	ID3D12GraphicsCommandList*	GetGraphicsCommandList() const { return CommandList->GraphicsCommandList.Get(); }
	ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const { return CommandList->GraphicsCommandList4.Get(); }
	ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const { return CommandList->GraphicsCommandList6.Get(); }

#ifdef NVIDIA_NSIGHT_AFTERMATH
	GFSDK_Aftermath_ContextHandle GetAftermathContextHandle() const { return pCommandList->AftermathContextHandle; }
#endif

	ID3D12GraphicsCommandList* operator->() const { return GetGraphicsCommandList(); }

	operator bool() const { return CommandList != nullptr; }

	void Close() { CommandList->Close(); }

	void Reset(D3D12CommandAllocator* CommandAllocator) { CommandList->Reset(CommandAllocator); }

	void SetSyncPoint(const D3D12CommandSyncPoint& SyncPoint)
	{
		CommandList->CommandAllocator->SetSyncPoint(SyncPoint);
	}

	CResourceState& GetResourceState(D3D12Resource* Resource)
	{
		return CommandList->ResourceStateTracker.GetResourceState(Resource);
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
	class D3D12CommandList : public D3D12LinkedDeviceChild
	{
	public:
		D3D12CommandList(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE Type);
		~D3D12CommandList();

		void Close();

		void Reset(D3D12CommandAllocator* CommandAllocator);

		void FlushResourceBarriers();

		const D3D12_COMMAND_LIST_TYPE Type;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>  GraphicsCommandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GraphicsCommandList4;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> GraphicsCommandList6;

#ifdef D3D12_DEBUG_RESOURCE_STATES
		Microsoft::WRL::ComPtr<ID3D12DebugCommandList> DebugCommandList;
#endif
#ifdef NVIDIA_NSIGHT_AFTERMATH
		GFSDK_Aftermath_ContextHandle AftermathContextHandle = nullptr;
#endif

		D3D12CommandAllocator* CommandAllocator;
		bool				   IsClosed;

		D3D12ResourceStateTracker ResourceStateTracker;
		ResourceBarrierBatch	  ResourceBarrierBatch;
	};

private:
	std::shared_ptr<D3D12CommandList> CommandList;
};
