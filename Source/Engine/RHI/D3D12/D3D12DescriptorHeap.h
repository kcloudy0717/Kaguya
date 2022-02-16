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

		operator ID3D12DescriptorHeap*() const noexcept { return DescriptorHeap.Get(); }

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

	class CDescriptorHeapManager : public D3D12LinkedDeviceChild
	{
	public: // Types
		using HeapOffsetRaw = decltype(D3D12_CPU_DESCRIPTOR_HANDLE::ptr);

	private: // Types
		struct SFreeRange
		{
			HeapOffsetRaw Start;
			HeapOffsetRaw End;
		};
		struct SHeapEntry
		{
			Arc<ID3D12DescriptorHeap> DescriptorHeap;
			std::list<SFreeRange>	  FreeList;
		};

		// Note: This data structure relies on the pointer validity guarantee of std::deque,
		// that as long as inserts/deletes are only on either end of the container, pointers
		// to elements continue to be valid. If trimming becomes an option, the free heap
		// list must be re-generated at that time.
		using THeapMap = std::deque<SHeapEntry>;

	public: // Methods
		explicit CDescriptorHeapManager(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   PageSize);

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateHeapSlot(UINT& OutDescriptorHeapIndex);

		void FreeHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE Offset, UINT DescriptorHeapIndex) noexcept;

	private: // Methods
		void AllocateHeap();

	private: // Members
		const D3D12_DESCRIPTOR_HEAP_DESC Desc;
		const UINT						 DescriptorSize;
		Mutex							 Mutex;

		THeapMap		Heaps;
		std::list<UINT> FreeHeaps;
	};

	struct D3D12DescriptorTableCache
	{
		// The pointer to the descriptor in the descriptor handle cache.
		D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
		UINT						 NumDescriptors;
	};

	class D3D12DescriptorHandleCache : public D3D12LinkedDeviceChild
	{
	public:
		static constexpr UINT DescriptorHandleLimit = 256;

		explicit D3D12DescriptorHandleCache(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type);

		void Reset();

		void ParseRootSignature(const D3D12RootSignature& RootSignature, D3D12_DESCRIPTOR_HEAP_TYPE Type);

		void StageDescriptors(
			UINT						RootParameterIndex,
			UINT						Offset,
			UINT						NumDescriptors,
			D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);

		template<RHI_PIPELINE_STATE_TYPE PsoType>
		UINT CommitDescriptors(
			CD3DX12_CPU_DESCRIPTOR_HANDLE& DestCpuHandle,
			CD3DX12_GPU_DESCRIPTOR_HANDLE& DestGpuHandle,
			ID3D12GraphicsCommandList*	   CommandList);

	private:
		const D3D12_DESCRIPTOR_HEAP_TYPE Type			= {};
		const UINT						 DescriptorSize = 0;

		// Each bit in the bit mask represents the index in the root signature
		// that contains a descriptor table.
		std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> DescriptorTableBitMask;

		// Each bit set in the bit mask represents a descriptor table
		// in the root signature that has changed since the last time the
		// descriptors were copied.
		std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> StaleDescriptorTableBitMask;

		D3D12DescriptorTableCache	DescriptorTableCaches[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandles[DescriptorHandleLimit]						= {};
	};

	class D3D12OnlineDescriptorHeap : public D3D12LinkedDeviceChild
	{
	public:
		explicit D3D12OnlineDescriptorHeap(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

		void Reset();

		void ParseGraphicsRootSignature(const D3D12RootSignature& RootSignature);
		void ParseComputeRootSignature(const D3D12RootSignature& RootSignature);

		void SetGraphicsDescriptorHandles(
			UINT						RootParameterIndex,
			UINT						Offset,
			UINT						NumDescriptors,
			D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);
		void SetComputeDescriptorHandles(
			UINT						RootParameterIndex,
			UINT						Offset,
			UINT						NumDescriptors,
			D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);

		void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CommandList);
		void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CommandList);

	private:
		Arc<ID3D12DescriptorHeap> InitializeDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

	private:
		const D3D12_DESCRIPTOR_HEAP_DESC Desc;
		UINT							 NumDescriptors = 0;
		Arc<ID3D12DescriptorHeap>		 DescriptorHeap;

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress = {};
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuBaseAddress = {};

		D3D12DescriptorHandleCache GraphicsHandleCache;
		D3D12DescriptorHandleCache ComputeHandleCache;
	};
} // namespace RHI
