#pragma once
#include "D3D12Core.h"

namespace RHI
{
	struct DescriptorIndexPool
	{
		// Removes the first element from the free list and returns its index
		UINT Allocate()
		{
			UINT NewIndex;
			if (!IndexQueue.empty())
			{
				NewIndex = IndexQueue.front();
				IndexQueue.pop();
			}
			else
			{
				NewIndex = Index++;
			}
			return NewIndex;
		}

		void Release(UINT Index)
		{
			IndexQueue.push(Index);
		}

		std::queue<UINT> IndexQueue;
		UINT			 Index = 0;
	};

	class D3D12DescriptorHeap : public D3D12LinkedDeviceChild
	{
	public:
		explicit D3D12DescriptorHeap(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

		void SetName(LPCWSTR Name) const { DescriptorHeap->SetName(Name); }

		operator ID3D12DescriptorHeap*() const { return DescriptorHeap.Get(); }

		void Allocate(
			D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
			UINT&						 Index);

		void Release(UINT Index);

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT Index) const noexcept;
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT Index) const noexcept;

	private:
		Arc<ID3D12DescriptorHeap> InitializeDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

	private:
		Arc<ID3D12DescriptorHeap>	DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC	Desc;
		D3D12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress = {};
		D3D12_GPU_DESCRIPTOR_HANDLE GpuBaseAddress = {};
		UINT						DescriptorSize = 0;

		DescriptorIndexPool IndexPool;
		Mutex				Mutex;
	};

	class D3D12DescriptorPage;

	// Represents a contiguous CPU descriptor handle
	class D3D12DescriptorArray
	{
	public:
		D3D12DescriptorArray() noexcept = default;
		explicit D3D12DescriptorArray(
			D3D12DescriptorPage*		Parent,
			D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle,
			UINT						Offset,
			UINT						NumDescriptors) noexcept;
		D3D12DescriptorArray(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept;
		D3D12DescriptorArray& operator=(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept;
		~D3D12DescriptorArray();

		D3D12DescriptorArray(const D3D12DescriptorArray&) = delete;
		D3D12DescriptorArray& operator=(const D3D12DescriptorArray&) = delete;

		[[nodiscard]] bool IsValid() const noexcept { return Parent != nullptr; }

		D3D12_CPU_DESCRIPTOR_HANDLE operator[](UINT Index) const noexcept;

		[[nodiscard]] UINT GetOffset() const noexcept { return Offset; }
		[[nodiscard]] UINT GetNumDescriptors() const noexcept { return NumDescriptors; }

	private:
		void InternalDestruct();

	private:
		D3D12DescriptorPage*		Parent				= nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle = {};
		UINT						Offset				= 0;
		UINT						NumDescriptors		= 0;
	};

	class D3D12DescriptorPage : public D3D12LinkedDeviceChild
	{
	public:
		explicit D3D12DescriptorPage(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

		[[nodiscard]] UINT GetDescriptorSize() const noexcept { return DescriptorSize; }

		std::optional<D3D12DescriptorArray> Allocate(UINT NumDescriptors);
		void								Release(D3D12DescriptorArray&& DescriptorArray);

	private:
		Arc<ID3D12DescriptorHeap> InitializeDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

	private:
		using OffsetType = UINT;
		using SizeType	 = UINT;

		struct FreeBlocksByOffsetPoolIter;
		struct FreeBlocksBySizePoolIter;
		using CFreeBlocksByOffsetPool = std::map<OffsetType, FreeBlocksBySizePoolIter>;
		using CFreeBlocksBySizePool	  = std::multimap<SizeType, FreeBlocksByOffsetPoolIter>;

		struct FreeBlocksByOffsetPoolIter
		{
			using IteratorType = CFreeBlocksByOffsetPool::iterator;

			explicit FreeBlocksByOffsetPoolIter(IteratorType Iterator)
				: Iterator(Iterator)
			{
			}

			IteratorType Iterator;
		};

		struct FreeBlocksBySizePoolIter
		{
			using IteratorType = CFreeBlocksBySizePool::iterator;

			explicit FreeBlocksBySizePoolIter(IteratorType Iterator)
				: Iterator(Iterator)
			{
			}

			IteratorType Iterator;
		};

		void AllocateFreeBlock(OffsetType Offset, SizeType Size);
		void FreeBlock(OffsetType Offset, SizeType Size);

	private:
		Arc<ID3D12DescriptorHeap>	DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC	Desc		   = {};
		D3D12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress = {};
		UINT						DescriptorSize = 0;

		CFreeBlocksByOffsetPool FreeBlocksByOffsetPool;
		CFreeBlocksBySizePool	SizePool;
		Mutex					Mutex;
	};

	class D3D12DescriptorAllocator : public D3D12LinkedDeviceChild
	{
	public:
		explicit D3D12DescriptorAllocator(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   PageSize) noexcept
			: D3D12LinkedDeviceChild(Parent)
			, Type(Type)
			, PageSize(PageSize)
		{
		}

		D3D12DescriptorArray Allocate(UINT NumDescriptors);

	private:
		D3D12_DESCRIPTOR_HEAP_TYPE						  Type;
		UINT											  PageSize;
		std::vector<std::unique_ptr<D3D12DescriptorPage>> DescriptorPages;
	};
} // namespace RHI
