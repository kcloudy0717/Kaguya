#include "D3D12DescriptorHeap.h"
#include "D3D12LinkedDevice.h"

void D3D12DescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT NumDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.Type						= Type;
	Desc.NumDescriptors				= NumDescriptors;
	Desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Desc.NodeMask					= 0;

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));

	CpuBaseAddress = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	GpuBaseAddress = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	DescriptorSize = GetParentLinkedDevice()->GetParentDevice()->GetSizeOfDescriptor(Type);

	IndexPool = DescriptorIndexPool(NumDescriptors);
}

void D3D12DescriptorHeap::Allocate(
	D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
	UINT&						 Index)
{
	std::scoped_lock _(Mutex);

	Index				= static_cast<UINT>(IndexPool.Allocate());
	CpuDescriptorHandle = this->GetCpuDescriptorHandle(Index);
	GpuDescriptorHandle = this->GetGpuDescriptorHandle(Index);
}

void D3D12DescriptorHeap::Release(UINT Index)
{
	std::scoped_lock _(Mutex);

	IndexPool.Release(static_cast<size_t>(Index));
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCpuDescriptorHandle(UINT Index) const noexcept
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuDescriptorHandle(UINT Index) const noexcept
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(GpuBaseAddress, static_cast<INT>(Index), DescriptorSize);
}

D3D12DescriptorPage::D3D12DescriptorPage(
	D3D12LinkedDevice*		   Parent,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT					   NumDescriptors)
	: D3D12LinkedDeviceChild(Parent)
{
	Desc.Type			= Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask		= 0;

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));

	CpuBaseAddress = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	DescriptorSize = GetParentLinkedDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	AllocateFreeBlock(0, NumDescriptors);
}

std::optional<D3D12DescriptorArray> D3D12DescriptorPage::Allocate(UINT NumDescriptors)
{
	if (NumDescriptors > Desc.NumDescriptors)
	{
		return std::nullopt;
	}

	// Get a block in the pool that is large enough to satisfy the required descriptor,
	// The function returns an iterator pointing to the key in the map container which is equivalent to k passed in the
	// parameter. In case k is not present in the map container, the function returns an iterator pointing to the
	// immediate next element which is just greater than k.
	auto iter = SizePool.lower_bound(NumDescriptors);
	if (iter == SizePool.end())
	{
		return std::nullopt;
	}

	// Query all information about this available block
	SizeType				   Size = iter->first;
	FreeBlocksByOffsetPoolIter OffsetIter(iter->second);
	OffsetType				   Offset = OffsetIter.Iterator->first;

	// Remove the block from the pool
	SizePool.erase(Size);
	FreeBlocksByOffsetPool.erase(OffsetIter.Iterator);

	// Allocate a available block
	OffsetType NewOffset = Offset + NumDescriptors;
	SizeType   NewSize	 = Size - NumDescriptors;

	// Return left over block to the pool
	if (NewSize > 0)
	{
		AllocateFreeBlock(NewOffset, NewSize);
	}

	Desc.NumDescriptors -= NumDescriptors;

	return D3D12DescriptorArray(
		this,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuBaseAddress, static_cast<INT>(Offset), DescriptorSize),
		Offset,
		NumDescriptors);
}

void D3D12DescriptorPage::Release(D3D12DescriptorArray&& DescriptorArray)
{
	FreeBlock(DescriptorArray.GetOffset(), DescriptorArray.GetNumDescriptors());
}

void D3D12DescriptorPage::AllocateFreeBlock(OffsetType Offset, SizeType Size)
{
	auto OffsetElement					 = FreeBlocksByOffsetPool.emplace(Offset, SizePool.begin());
	auto SizeElement					 = SizePool.emplace(Size, OffsetElement.first);
	OffsetElement.first->second.Iterator = SizeElement;
}

void D3D12DescriptorPage::FreeBlock(OffsetType Offset, SizeType Size)
{
	// upper_bound returns end if it there is no next block
	auto NextIter = FreeBlocksByOffsetPool.upper_bound(Offset);
	auto PrevIter = NextIter;
	if (PrevIter != FreeBlocksByOffsetPool.begin())
	{
		// there is no free block in front of this one
		--PrevIter;
	}
	else
	{
		PrevIter = FreeBlocksByOffsetPool.end();
	}

	Desc.NumDescriptors += Size;

	if (PrevIter != FreeBlocksByOffsetPool.end())
	{
		OffsetType PrevOffset = PrevIter->first;
		SizeType   PrevSize	  = PrevIter->second.Iterator->first;
		if (Offset == PrevOffset + PrevSize)
		{
			// Merge previous and current block
			// PrevBlock.Offset					CurrentBlock.Offset
			// |								|
			// |<--------PrevBlock.Size-------->|<--------CurrentBlock.Size-------->|

			Offset = PrevOffset;
			Size += PrevSize;

			// Remove this block now that it has been merged
			// Erase order needs to be size the offset, because offset iter still references element in the offset pool
			SizePool.erase(PrevIter->second.Iterator);
			FreeBlocksByOffsetPool.erase(PrevOffset);
		}
	}

	if (NextIter != FreeBlocksByOffsetPool.end())
	{
		OffsetType NextOffset = NextIter->first;
		SizeType   NextSize	  = NextIter->second.Iterator->first;
		if (Offset + Size == NextOffset)
		{
			// Merge current and next block
			// CurrentBlock.Offset				   NextBlock.Offset
			// |								   |
			// |<--------CurrentBlock.Size-------->|<--------NextBlock.Size-------->

			// only increase the size of the block
			Size += NextSize;

			// Remove this block now that it has been merged
			// Erase order needs to be size the offset, because offset iter still references element in the offset pool
			SizePool.erase(NextIter->second.Iterator);
			FreeBlocksByOffsetPool.erase(NextOffset);
		}
	}

	AllocateFreeBlock(Offset, Size);
}

D3D12DescriptorArray::~D3D12DescriptorArray()
{
	if (IsValid())
	{
		Parent->Release(std::move(*this));

		Parent				= nullptr;
		CpuDescriptorHandle = {};
		Offset				= 0;
		NumDescriptors		= 0;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorArray::operator[](UINT Index) const noexcept
{
	assert(Index < NumDescriptors);
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuDescriptorHandle, static_cast<INT>(Index), Parent->GetDescriptorSize());
}

D3D12DescriptorArray D3D12DescriptorAllocator::Allocate(UINT NumDescriptors)
{
	D3D12DescriptorArray DescriptorArray;

	for (auto iter = DescriptorPages.begin(); iter != DescriptorPages.end(); ++iter)
	{
		auto OptDescriptorArray = iter->get()->Allocate(NumDescriptors);
		if (OptDescriptorArray.has_value())
		{
			DescriptorArray = std::move(OptDescriptorArray.value());
			break;
		}
	}

	if (!DescriptorArray.IsValid())
	{
		auto& NewPage = DescriptorPages.emplace_back(
			std::make_unique<D3D12DescriptorPage>(GetParentLinkedDevice(), Type, PageSize));
		DescriptorArray = NewPage->Allocate(NumDescriptors).value();
	}

	return DescriptorArray;
}

#if 0
void D3D12DescriptorHandleCache::StageDescriptors(
	UINT						RootParameterIndex,
	UINT						Offset,
	UINT						NumDescriptors,
	D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
{
	assert(
		RootParameterIndex < D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT &&
		"Root parameter index exceeds the max descriptor table index");

	D3D12DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];

	assert(
		Offset + NumDescriptors <= DescriptorTableCache.NumDescriptors &&
		"Number of descriptors exceeds the number of descripotrs in the descriptor table");

	D3D12_CPU_DESCRIPTOR_HANDLE* DestDescriptor = DescriptorTableCache.BaseDescriptor + Offset;
	for (UINT i = 0; i < NumDescriptors; ++i)
	{
		DestDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(SrcDescriptor, static_cast<INT>(i), DescriptorSize);
	}

	StaleDescriptorTableBitMask.set(RootParameterIndex, true);
}

UINT D3D12DescriptorHandleCache::CommitDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE& DestCpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE& DestGpuHandle,
	ID3D12GraphicsCommandList*	   CommandList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
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

		(CommandList->*SetFunc)(RootParameterIndex, DestGpuHandle);

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

void D3D12DynamicDescriptorHeap::Initialize()
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.Type						= Type;
	Desc.NumDescriptors				= NumDescriptors;
	Desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Desc.NodeMask					= 0;

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&DescriptorHeap)));

	CpuBaseAddress = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	GpuBaseAddress = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	GraphicsHandleCache.Initialize(Type, GetParentLinkedDevice()->GetParentDevice()->GetSizeOfDescriptor(Type));
	ComputeHandleCache.Initialize(Type, GetParentLinkedDevice()->GetParentDevice()->GetSizeOfDescriptor(Type));
}

void D3D12DynamicDescriptorHeap::Reset()
{
	CpuBaseAddress = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	GpuBaseAddress = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	GraphicsHandleCache.Reset();
	ComputeHandleCache.Reset();
}

void D3D12DynamicDescriptorHeap::ParseGraphicsRootSignature(const D3D12RootSignature& RootSignature)
{
	GraphicsHandleCache.ParseRootSignature(RootSignature, Type);
}

void D3D12DynamicDescriptorHeap::ParseComputeRootSignature(const D3D12RootSignature& RootSignature)
{
	ComputeHandleCache.ParseRootSignature(RootSignature, Type);
}

void D3D12DynamicDescriptorHeap::SetGraphicsDescriptorHandles(
	UINT						RootParameterIndex,
	UINT						Offset,
	UINT						NumDescriptors,
	D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
{
	GraphicsHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
}

void D3D12DynamicDescriptorHeap::SetComputeDescriptorHandles(
	UINT						RootParameterIndex,
	UINT						Offset,
	UINT						NumDescriptors,
	D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
{
	ComputeHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
}

void D3D12DynamicDescriptorHeap::CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CommandList)
{
	UINT NumDescriptorsCommitted = GraphicsHandleCache.CommitDescriptors(
		CpuBaseAddress,
		GpuBaseAddress,
		CommandList,
		&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	this->NumDescriptors -= NumDescriptorsCommitted;
}

void D3D12DynamicDescriptorHeap::CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CommandList)
{
	UINT NumDescriptorsCommitted = ComputeHandleCache.CommitDescriptors(
		CpuBaseAddress,
		GpuBaseAddress,
		CommandList,
		&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	this->NumDescriptors -= NumDescriptorsCommitted;
}
#endif
