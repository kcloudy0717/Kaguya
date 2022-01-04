#pragma once
#include "D3D12Common.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandContext.h"

class D3D12LinkedDevice : public D3D12DeviceChild
{
public:
	explicit D3D12LinkedDevice(D3D12Device* Parent);
	~D3D12LinkedDevice();

	[[nodiscard]] ID3D12Device*				GetDevice() const;
	[[nodiscard]] ID3D12Device5*			GetDevice5() const;
	[[nodiscard]] D3D12CommandQueue*		GetCommandQueue(RHID3D12CommandQueueType Type);
	[[nodiscard]] D3D12CommandQueue*		GetGraphicsQueue();
	[[nodiscard]] D3D12CommandQueue*		GetAsyncComputeQueue();
	[[nodiscard]] D3D12CommandQueue*		GetCopyQueue1();
	[[nodiscard]] D3D12DescriptorAllocator& GetRtvAllocator() noexcept;
	[[nodiscard]] D3D12DescriptorAllocator& GetDsvAllocator() noexcept;
	[[nodiscard]] D3D12DescriptorHeap&		GetResourceDescriptorHeap() noexcept;
	[[nodiscard]] D3D12DescriptorHeap&		GetSamplerDescriptorHeap() noexcept;
	// clang-format off
	template <typename ViewDesc> D3D12DescriptorHeap& GetDescriptorHeap() noexcept;
	template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	// clang-format on
	[[nodiscard]] D3D12CommandContext& GetCommandContext(UINT ThreadIndex = 0);
	[[nodiscard]] D3D12CommandContext& GetAsyncComputeCommandContext(UINT ThreadIndex = 0);
	[[nodiscard]] D3D12CommandContext& GetCopyContext1();

	void WaitIdle();

	void			BeginResourceUpload();
	D3D12SyncHandle EndResourceUpload(bool WaitForCompletion);

	void Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource);
	void Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* Resource);

private:
	D3D12CommandQueue GraphicsQueue;
	D3D12CommandQueue AsyncComputeQueue;
	D3D12CommandQueue CopyQueue1;
	D3D12CommandQueue CopyQueue2;

	D3D12DescriptorAllocator RtvAllocator;
	D3D12DescriptorAllocator DsvAllocator;
	D3D12DescriptorHeap		 ResourceDescriptorHeap;
	D3D12DescriptorHeap		 SamplerDescriptorHeap;

	std::vector<std::unique_ptr<D3D12CommandContext>> AvailableCommandContexts;
	std::vector<std::unique_ptr<D3D12CommandContext>> AvailableAsyncCommandContexts;
	std::unique_ptr<D3D12CommandContext>			  CopyContext1;
	std::unique_ptr<D3D12CommandContext>			  CopyContext2;

	D3D12SyncHandle										UploadSyncHandle;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> TrackedResources;
};
