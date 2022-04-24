#pragma once
#include "D3D12Core.h"
#include "D3D12CommandQueue.h"
#include "D3D12Profiler.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorHeap.h"

namespace RHI
{
	class D3D12LinkedDevice : public D3D12DeviceChild
	{
	public:
		explicit D3D12LinkedDevice(
			D3D12Device*  Parent,
			D3D12NodeMask NodeMask);
		~D3D12LinkedDevice();

		[[nodiscard]] ID3D12Device*		   GetDevice() const;
		[[nodiscard]] ID3D12Device5*	   GetDevice5() const;
		[[nodiscard]] D3D12NodeMask		   GetNodeMask() const noexcept { return NodeMask; }
		[[nodiscard]] D3D12CommandQueue*   GetCommandQueue(RHID3D12CommandQueueType Type);
		[[nodiscard]] D3D12CommandQueue*   GetGraphicsQueue();
		[[nodiscard]] D3D12CommandQueue*   GetAsyncComputeQueue();
		[[nodiscard]] D3D12CommandQueue*   GetCopyQueue1();
		[[nodiscard]] D3D12Profiler*	   GetProfiler();
		[[nodiscard]] D3D12DescriptorHeap& GetResourceDescriptorHeap() noexcept;
		[[nodiscard]] D3D12DescriptorHeap& GetSamplerDescriptorHeap() noexcept;
		// clang-format off
		template<typename ViewDesc> CDescriptorHeapManager& GetHeapManager() noexcept;
		template<> CDescriptorHeapManager& GetHeapManager<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return RtvHeapManager; }
		template<> CDescriptorHeapManager& GetHeapManager<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return DsvHeapManager; }
		template<typename ViewDesc> D3D12DescriptorHeap& GetDescriptorHeap() noexcept;
		template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_CONSTANT_BUFFER_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
		template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
		template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
		template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_SAMPLER_DESC>() noexcept { return SamplerDescriptorHeap; }
		// clang-format on
		[[nodiscard]] D3D12CommandContext& GetCommandContext(UINT ThreadIndex = 0);
		[[nodiscard]] D3D12CommandContext& GetAsyncComputeCommandContext(UINT ThreadIndex = 0);
		[[nodiscard]] D3D12CommandContext& GetCopyContext1();

		void OnBeginFrame();
		void OnEndFrame();

		[[nodiscard]] D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& Desc) const;
		[[nodiscard]] bool							 ResourceSupport4KiBAlignment(D3D12_RESOURCE_DESC* Desc) const;

		void WaitIdle();

		void			BeginResourceUpload();
		D3D12SyncHandle EndResourceUpload(bool WaitForCompletion);

		void Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource);
		void Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* Resource);
		void Upload(const void* Data, UINT64 SizeInBytes, ID3D12Resource* Resource);

	private:
		D3D12NodeMask	  NodeMask;
		D3D12CommandQueue GraphicsQueue;
		D3D12CommandQueue AsyncComputeQueue;
		D3D12CommandQueue CopyQueue1;
		D3D12CommandQueue CopyQueue2;
		D3D12Profiler	  Profiler;

		CDescriptorHeapManager RtvHeapManager;
		CDescriptorHeapManager DsvHeapManager;
		D3D12DescriptorHeap	   ResourceDescriptorHeap;
		D3D12DescriptorHeap	   SamplerDescriptorHeap;

		std::vector<D3D12CommandContext> AvailableCommandContexts;
		std::vector<D3D12CommandContext> AvailableAsyncCommandContexts;
		D3D12CommandContext				 CopyContext1;
		D3D12CommandContext				 CopyContext2;

		struct ResourceAllocationInfoTable
		{
			RwLock													Mutex;
			std::unordered_map<u64, D3D12_RESOURCE_ALLOCATION_INFO> Table;
		} mutable ResourceAllocationInfoTable;

		D3D12SyncHandle					 UploadSyncHandle;
		std::vector<Arc<ID3D12Resource>> TrackedResources;
	};
} // namespace RHI
