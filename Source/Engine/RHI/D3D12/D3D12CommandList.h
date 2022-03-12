#pragma once
#include "D3D12Core.h"
#include "D3D12Resource.h"

namespace RHI
{
	class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
	{
	public:
		D3D12CommandAllocatorPool() noexcept = default;
		explicit D3D12CommandAllocatorPool(
			D3D12LinkedDevice*		Parent,
			D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

		[[nodiscard]] Arc<ID3D12CommandAllocator> RequestCommandAllocator();

		void DiscardCommandAllocator(
			Arc<ID3D12CommandAllocator> CommandAllocator,
			D3D12SyncHandle				SyncHandle);

	private:
		D3D12_COMMAND_LIST_TYPE					CommandListType;
		CFencePool<Arc<ID3D12CommandAllocator>> CommandAllocatorPool;
	};

	// https://www.youtube.com/watch?v=nmB2XMasz2o, Resource state tracking
	struct PendingResourceBarrier
	{
		D3D12Resource*		  Resource;
		D3D12_RESOURCE_STATES State;
		UINT				  Subresource;
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

		D3D12CommandListHandle(D3D12CommandListHandle&&) noexcept = default;
		D3D12CommandListHandle& operator=(D3D12CommandListHandle&&) noexcept = default;

		D3D12CommandListHandle(const D3D12CommandListHandle&) = delete;
		D3D12CommandListHandle& operator=(const D3D12CommandListHandle&) = delete;

		[[nodiscard]] ID3D12CommandList*		  GetCommandList() const noexcept { return GraphicsCommandList.Get(); }
		[[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const noexcept { return GraphicsCommandList.Get(); }
		[[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const noexcept { return GraphicsCommandList4.Get(); }
		[[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const noexcept { return GraphicsCommandList6.Get(); }
		ID3D12GraphicsCommandList*				  operator->() const noexcept { return GetGraphicsCommandList(); }

		void Open(
			ID3D12CommandAllocator* CommandAllocator);

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

		// Resource barrier helpers
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

	private:
		friend class D3D12CommandQueue;

		static constexpr UINT NumBatches = 16;

		D3D12_COMMAND_LIST_TYPE			Type;
		Arc<ID3D12GraphicsCommandList>	GraphicsCommandList;
		Arc<ID3D12GraphicsCommandList4> GraphicsCommandList4;
		Arc<ID3D12GraphicsCommandList6> GraphicsCommandList6;
#ifdef LUNA_D3D12_DEBUG_RESOURCE_STATES
		Arc<ID3D12DebugCommandList> DebugCommandList;
#endif
		D3D12ResourceStateTracker ResourceStateTracker;
		D3D12_RESOURCE_BARRIER	  ResourceBarriers[NumBatches] = {};
		UINT					  NumResourceBarriers		   = 0;
	};
} // namespace RHI
