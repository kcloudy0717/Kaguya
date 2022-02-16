#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"
#include "D3D12LinkedDevice.h"

namespace RHI
{
	D3D12DescriptorHeap::D3D12DescriptorHeap(
		D3D12LinkedDevice*		   Parent,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT					   NumDescriptors)
		: D3D12LinkedDeviceChild(Parent)
		, DescriptorHeap(InitializeDescriptorHeap(Type, NumDescriptors))
		, Desc(DescriptorHeap->GetDesc())
		, CpuBaseAddress(DescriptorHeap->GetCPUDescriptorHandleForHeapStart())
		, GpuBaseAddress(DescriptorHeap->GetGPUDescriptorHandleForHeapStart())
		, DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
	{
	}

	void D3D12DescriptorHeap::Allocate(
		D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
		UINT&						 Index)
	{
		MutexGuard Guard(Mutex);
		Index				= IndexPool.Allocate();
		CpuDescriptorHandle = this->GetCpuDescriptorHandle(Index);
		GpuDescriptorHandle = this->GetGpuDescriptorHandle(Index);
	}

	void D3D12DescriptorHeap::Release(UINT Index)
	{
		MutexGuard Guard(Mutex);
		IndexPool.Release(Index);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCpuDescriptorHandle(UINT Index) const noexcept
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuDescriptorHandle(UINT Index) const noexcept
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(GpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
	}

	Arc<ID3D12DescriptorHeap> D3D12DescriptorHeap::InitializeDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT					   NumDescriptors)
	{
		Arc<ID3D12DescriptorHeap>  DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC Desc = {
			.Type			= Type,
			.NumDescriptors = NumDescriptors,
			.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask		= Parent->GetNodeMask()
		};
		VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));
		return DescriptorHeap;
	}

	D3D12OnlineDescriptorHeap::D3D12OnlineDescriptorHeap(
		D3D12LinkedDevice*		   Parent,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT					   NumDescriptors)
		: D3D12LinkedDeviceChild(Parent)
		, Desc({ Type,
				 NumDescriptors,
				 D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				 Parent->GetNodeMask() })
		, NumDescriptors(NumDescriptors)
		, DescriptorHeap(InitializeDescriptorHeap(Type, NumDescriptors))
		, CpuBaseAddress(DescriptorHeap->GetCPUDescriptorHandleForHeapStart())
		, GpuBaseAddress(DescriptorHeap->GetGPUDescriptorHandleForHeapStart())
		, GraphicsHandleCache(Parent, Type)
		, ComputeHandleCache(Parent, Type)
	{
	}

	D3D12DescriptorHandleCache::D3D12DescriptorHandleCache(
		D3D12LinkedDevice*		   Parent,
		D3D12_DESCRIPTOR_HEAP_TYPE Type)
		: D3D12LinkedDeviceChild(Parent)
		, Type(Type)
		, DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
	{
	}

	void D3D12DescriptorHandleCache::Reset()
	{
		DescriptorTableBitMask.reset();
		StaleDescriptorTableBitMask.reset();

		for (auto& DescriptorTableCache : DescriptorTableCaches)
		{
			DescriptorTableCache = {};
		}
	}

	void D3D12DescriptorHandleCache::ParseRootSignature(const D3D12RootSignature& RootSignature, D3D12_DESCRIPTOR_HEAP_TYPE Type)
	{
		StaleDescriptorTableBitMask.reset();

		DescriptorTableBitMask = RootSignature.GetDescriptorTableBitMask(Type);
		if (!DescriptorTableBitMask.any())
		{
			return;
		}

		UINT DescriptorOffset = 0;
		for (size_t i = 0; i < DescriptorTableBitMask.size(); ++i)
		{
			if (DescriptorTableBitMask.test(i))
			{
				UINT RootParameterIndex = static_cast<UINT>(i);
				if (RootParameterIndex < RootSignature.GetNumParameters())
				{
					UINT NumDescriptors = RootSignature.GetNumDescriptors(RootParameterIndex);

					D3D12DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];
					DescriptorTableCache.NumDescriptors				= NumDescriptors;
					DescriptorTableCache.BaseDescriptor				= DescriptorHandles + DescriptorOffset;

					DescriptorOffset += NumDescriptors;
				}
			}
		}

		// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
		assert(DescriptorOffset <= DescriptorHandleLimit && "Exceeded user-supplied maximum cache size");
	}

	void D3D12DescriptorHandleCache::StageDescriptors(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		assert(RootParameterIndex < D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT && "Root parameter index exceeds the max descriptor table index");

		D3D12DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];

		assert(Offset + NumDescriptors <= DescriptorTableCache.NumDescriptors && "Number of descriptors exceeds the number of descripotrs in the descriptor table");

		D3D12_CPU_DESCRIPTOR_HANDLE* DestDescriptor = DescriptorTableCache.BaseDescriptor + Offset;
		for (UINT i = 0; i < NumDescriptors; ++i)
		{
			DestDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(SrcDescriptor, static_cast<INT>(i), DescriptorSize);
		}

		StaleDescriptorTableBitMask.set(RootParameterIndex, true);
	}

	template<RHI_PIPELINE_STATE_TYPE PsoType>
	UINT D3D12DescriptorHandleCache::CommitDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE& DestCpuHandle,
		CD3DX12_GPU_DESCRIPTOR_HANDLE& DestGpuHandle,
		ID3D12GraphicsCommandList*	   CommandList)
	{
		ID3D12Device* Device				   = GetParentLinkedDevice()->GetDevice();
		UINT		  NumStaleDescriptorTables = 0;
		UINT		  RootIndices[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT];

		for (size_t i = 0; i < StaleDescriptorTableBitMask.size(); ++i)
		{
			if (StaleDescriptorTableBitMask.test(i))
			{
				RootIndices[i] = static_cast<UINT>(i);

				NumStaleDescriptorTables++;
			}
		}

		UINT NumDescriptorsCommitted = 0;
		for (UINT i = 0; i < NumStaleDescriptorTables; ++i)
		{
			UINT						 RootParameterIndex = RootIndices[i];
			UINT						 NumDescriptors		= DescriptorTableCaches[RootParameterIndex].NumDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* DescriptorHandle	= DescriptorTableCaches[RootParameterIndex].BaseDescriptor;

			D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[] = { DestCpuHandle };
			UINT						pDestDescriptorRangeSizes[]	 = { NumDescriptors };
			Device->CopyDescriptors(
				1,
				pDestDescriptorRangeStarts,
				pDestDescriptorRangeSizes,
				NumDescriptors,
				DescriptorHandle,
				nullptr,
				Type);

			(CommandList->*D3D12DynamicResourceTableTraits<PsoType>::Bind())(RootParameterIndex, DestGpuHandle);

			// Offset current descriptor handles.
			DestCpuHandle.Offset(static_cast<INT>(NumDescriptors), DescriptorSize);
			DestGpuHandle.Offset(static_cast<INT>(NumDescriptors), DescriptorSize);
			NumDescriptorsCommitted += NumDescriptors;

			// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new
			// descriptor.
			StaleDescriptorTableBitMask.flip(i);
		}

		return NumDescriptorsCommitted;
	}

	void D3D12OnlineDescriptorHeap::Reset()
	{
		CpuBaseAddress = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		GpuBaseAddress = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		GraphicsHandleCache.Reset();
		ComputeHandleCache.Reset();
	}

	void D3D12OnlineDescriptorHeap::ParseGraphicsRootSignature(const D3D12RootSignature& RootSignature)
	{
		GraphicsHandleCache.ParseRootSignature(RootSignature, Desc.Type);
	}

	void D3D12OnlineDescriptorHeap::ParseComputeRootSignature(const D3D12RootSignature& RootSignature)
	{
		ComputeHandleCache.ParseRootSignature(RootSignature, Desc.Type);
	}

	void D3D12OnlineDescriptorHeap::SetGraphicsDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		GraphicsHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
	}

	void D3D12OnlineDescriptorHeap::SetComputeDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		ComputeHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
	}

	void D3D12OnlineDescriptorHeap::CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CommandList)
	{
		UINT NumDescriptorsCommitted = GraphicsHandleCache.CommitDescriptors<RHI_PIPELINE_STATE_TYPE::Graphics>(
			CpuBaseAddress,
			GpuBaseAddress,
			CommandList);
		this->NumDescriptors -= NumDescriptorsCommitted;
	}

	void D3D12OnlineDescriptorHeap::CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CommandList)
	{
		UINT NumDescriptorsCommitted = ComputeHandleCache.CommitDescriptors<RHI_PIPELINE_STATE_TYPE::Graphics>(
			CpuBaseAddress,
			GpuBaseAddress,
			CommandList);
		this->NumDescriptors -= NumDescriptorsCommitted;
	}

	Arc<ID3D12DescriptorHeap> D3D12OnlineDescriptorHeap::InitializeDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UINT					   NumDescriptors)
	{
		Arc<ID3D12DescriptorHeap>  DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_DESC Desc = {
			.Type			= Type,
			.NumDescriptors = NumDescriptors,
			.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask		= Parent->GetNodeMask()
		};
		VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));
		return DescriptorHeap;
	}

	CDescriptorHeapManager::CDescriptorHeapManager(D3D12LinkedDevice* Parent, D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT PageSize)
		: D3D12LinkedDeviceChild(Parent)
		, Desc({ Type,
				 PageSize,
				 D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				 Parent->GetNodeMask() })
		, DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
	{
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CDescriptorHeapManager::AllocateHeapSlot(UINT& OutDescriptorHeapIndex)
	{
		MutexGuard Guard(Mutex);

		if (FreeHeaps.empty())
		{
			AllocateHeap(); // throw( _com_error )
		}
		assert(!FreeHeaps.empty());
		UINT		Index	  = FreeHeaps.front();
		SHeapEntry& HeapEntry = Heaps[Index];
		assert(!HeapEntry.FreeList.empty());
		SFreeRange&					Range = *HeapEntry.FreeList.begin();
		D3D12_CPU_DESCRIPTOR_HANDLE Ret	  = { Range.Start };
		Range.Start += DescriptorSize;

		if (Range.Start == Range.End)
		{
			HeapEntry.FreeList.pop_front();
			if (HeapEntry.FreeList.empty())
			{
				FreeHeaps.pop_front();
			}
		}
		OutDescriptorHeapIndex = Index;
		return Ret;
	}

	void CDescriptorHeapManager::FreeHeapSlot(D3D12_CPU_DESCRIPTOR_HANDLE Offset, UINT DescriptorHeapIndex) noexcept
	{
		MutexGuard Guard(Mutex);
		try
		{
			assert(DescriptorHeapIndex < Heaps.size());
			SHeapEntry& HeapEntry = Heaps[DescriptorHeapIndex];

			SFreeRange NewRange = {
				Offset.ptr,
				Offset.ptr + DescriptorSize
			};

			bool bFound = false;
			for (auto Iter = HeapEntry.FreeList.begin(), End = HeapEntry.FreeList.end();
				 Iter != End && !bFound;
				 ++Iter)
			{
				SFreeRange& Range = *Iter;
				assert(Range.Start <= Range.End);
				if (Range.Start == Offset.ptr + DescriptorSize)
				{
					Range.Start = Offset.ptr;
					bFound		= true;
				}
				else if (Range.End == Offset.ptr)
				{
					Range.End += DescriptorSize;
					bFound = true;
				}
				else
				{
					assert(Range.End < Offset.ptr || Range.Start > Offset.ptr);
					if (Range.Start > Offset.ptr)
					{
						HeapEntry.FreeList.insert(Iter, NewRange); // throw( bad_alloc )
						bFound = true;
					}
				}
			}

			if (!bFound)
			{
				if (HeapEntry.FreeList.empty())
				{
					FreeHeaps.push_back(DescriptorHeapIndex); // throw( bad_alloc )
				}
				HeapEntry.FreeList.push_back(NewRange); // throw( bad_alloc )
			}
		}
		catch (std::bad_alloc&)
		{
			// Do nothing - there will be slots that can no longer be reclaimed.
		}
	}

	void CDescriptorHeapManager::AllocateHeap()
	{
		SHeapEntry NewEntry;
		VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&NewEntry.DescriptorHeap)));
		D3D12_CPU_DESCRIPTOR_HANDLE HeapBase = NewEntry.DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		NewEntry.FreeList.push_back({ HeapBase.ptr, HeapBase.ptr + Desc.NumDescriptors * DescriptorSize }); // throw( bad_alloc )

		Heaps.emplace_back(std::move(NewEntry));				  // throw( bad_alloc )
		FreeHeaps.push_back(static_cast<UINT>(Heaps.size() - 1)); // throw( bad_alloc )
	}
} // namespace RHI
