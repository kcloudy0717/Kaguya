#pragma once
#include <queue>
#include <bitset>
#include <list>
#include "D3D12Core.h"

namespace RHI
{
	class D3D12DescriptorHeap;

	struct DeferredDeleteDescriptor
	{
		void Release();

		D3D12DescriptorHeap* OwningHeap = nullptr;
		UINT				 Index;
	};

	// Dynamic resource binding heap, descriptor index are maintained in a free list
	class D3D12DescriptorHeap : public D3D12LinkedDeviceChild
	{
	public:
		D3D12DescriptorHeap() noexcept = default;
		explicit D3D12DescriptorHeap(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   NumDescriptors);

		[[nodiscard]] ID3D12DescriptorHeap* GetApiHandle() const noexcept { return DescriptorHeap; }

		void SetName(LPCWSTR Name) const { DescriptorHeap->SetName(Name); }

		void Allocate(
			D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
			UINT&						 Index);

		void Release(UINT Index);

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT Index) const noexcept;
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT Index) const noexcept;

	private:
		Arc<ID3D12DescriptorHeap>	DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC	Desc;
		D3D12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress = {};
		D3D12_GPU_DESCRIPTOR_HANDLE GpuBaseAddress = {};
		UINT						DescriptorSize = 0;

		struct DescriptorIndexPool
		{
			// Removes the first element from the free list and returns its index
			UINT Allocate()
			{
				MutexGuard Guard(*IndexMutex);

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
				MutexGuard Guard(*IndexMutex);
				IndexQueue.push(Index);
			}

			std::unique_ptr<Mutex> IndexMutex = std::make_unique<Mutex>();
			std::queue<UINT>	   IndexQueue;
			UINT				   Index = 0;
		} IndexPool;
	};

	// Used for RTV/DSV Heaps, Source: https://github.com/microsoft/D3D12TranslationLayer/blob/master/include/ImmediateContext.hpp#L320
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
		CDescriptorHeapManager() noexcept = default;
		explicit CDescriptorHeapManager(
			D3D12LinkedDevice*		   Parent,
			D3D12_DESCRIPTOR_HEAP_TYPE Type,
			UINT					   PageSize);

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateHeapSlot(UINT& OutDescriptorHeapIndex);

		void FreeHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE Offset, UINT DescriptorHeapIndex) noexcept;

	private: // Methods
		void AllocateHeap();

	private: // Members
		D3D12_DESCRIPTOR_HEAP_DESC Desc;
		UINT					   DescriptorSize;
		std::unique_ptr<Mutex>	   HeapMutex;

		THeapMap		Heaps;
		std::list<UINT> FreeHeaps;
	};

	// Code below was used to simulate D3D11-ish kind binding model
	// No longer used
	namespace Internal
	{
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

			D3D12DescriptorHandleCache() noexcept = default;
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
			D3D12_DESCRIPTOR_HEAP_TYPE Type			  = {};
			UINT					   DescriptorSize = 0;

			// Each bit in the bit mask represents the index in the root signature
			// that contains a descriptor table.
			std::bitset<KAGUYA_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> DescriptorTableBitMask;

			// Each bit set in the bit mask represents a descriptor table
			// in the root signature that has changed since the last time the
			// descriptors were copied.
			std::bitset<KAGUYA_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> StaleDescriptorTableBitMask;

			D3D12DescriptorTableCache	DescriptorTableCaches[KAGUYA_RHI_D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT] = {};
			D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandles[DescriptorHandleLimit]								   = {};
		};
	} // namespace Internal

	class D3D12OnlineDescriptorHeap : public D3D12LinkedDeviceChild
	{
	public:
		D3D12OnlineDescriptorHeap() noexcept = default;
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
		D3D12_DESCRIPTOR_HEAP_DESC Desc;
		UINT					   NumDescriptors = 0;
		Arc<ID3D12DescriptorHeap>  DescriptorHeap;

		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress = {};
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuBaseAddress = {};

		Internal::D3D12DescriptorHandleCache GraphicsHandleCache;
		Internal::D3D12DescriptorHandleCache ComputeHandleCache;
	};
} // namespace RHI
