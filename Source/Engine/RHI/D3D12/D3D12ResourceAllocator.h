#pragma once
#include "D3D12Core.h"

namespace RHI
{
	struct D3D12Allocation
	{
		ID3D12Resource*			  Resource; // Weak reference
		UINT64					  Offset;
		UINT64					  Size;
		BYTE*					  CpuVirtualAddress;
		D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
	};

	class D3D12LinearAllocator : public D3D12LinkedDeviceChild
	{
	public:
		static constexpr UINT64 CpuAllocatorPageSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		D3D12LinearAllocator() noexcept = default;
		explicit D3D12LinearAllocator(D3D12LinkedDevice* Parent);

		D3D12LinearAllocator(D3D12LinearAllocator&&) noexcept = default;
		D3D12LinearAllocator& operator=(D3D12LinearAllocator&&) noexcept = default;

		D3D12LinearAllocator(const D3D12LinearAllocator&) = delete;
		D3D12LinearAllocator& operator=(const D3D12LinearAllocator&) = delete;

		// Versions all the current constant data with SyncHandle to ensure memory is not overriden when it GPU uses it
		void Version(D3D12SyncHandle SyncHandle);

		[[nodiscard]] D3D12Allocation Allocate(
			UINT64 Size,
			UINT   Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		template<typename T>
		[[nodiscard]] D3D12Allocation Allocate(
			const T& Data,
			UINT	 Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
		{
			D3D12Allocation Allocation = Allocate(sizeof(T), Alignment);
			std::memcpy(Allocation.CpuVirtualAddress, &Data, sizeof(T));
			return Allocation;
		}

	private:
		class Page
		{
		public:
			explicit Page(
				Arc<ID3D12Resource> Resource,
				UINT64				PageSize);
			~Page();

			std::optional<D3D12Allocation> Suballocate(UINT64 Size, UINT Alignment);

			void Reset();

		private:
			Arc<ID3D12Resource>		  Resource;
			UINT64					  Offset;
			UINT64					  PageSize;
			BYTE*					  CpuVirtualAddress;
			D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
		};

		[[nodiscard]] Page* RequestPage();

		[[nodiscard]] std::unique_ptr<Page> CreateNewPage(UINT64 PageSize) const;

		void DiscardPages(UINT64 FenceValue, const std::vector<Page*>& Pages);

	private:
		std::vector<std::unique_ptr<Page>>	 PagePool;
		std::queue<std::pair<UINT64, Page*>> RetiredPages;
		std::queue<Page*>					 AvailablePages;

		D3D12SyncHandle	   SyncHandle;
		Page*			   CurrentPage = nullptr;
		std::vector<Page*> RetiredPageList;
	};
} // namespace RHI
