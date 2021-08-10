#include "D3D12DescriptorHeap.h"
#include "D3D12LinkedDevice.h"

static AutoConsoleVariable<int> CVar_DescriptorPageSize("D3D12.DescriptorPageSize", "Descriptor Page Size", 4096);

void D3D12DynamicResourceDescriptorHeap::Initialize(
	UINT					   NumDescriptors,
	bool					   ShaderVisible,
	D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	// If you recorded a CPU descriptor handle into the command list (render target or depth stencil) then that
	// descriptor can be reused immediately after the Set call, if you recorded a GPU descriptor handle into the command
	// list (everything else) then that descriptor cannot be reused until gpu is done referencing them
	Desc = { .Type			 = Type,
			 .NumDescriptors = NumDescriptors,
			 .Flags	   = ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			 .NodeMask = 0 };

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pDescriptorHeap)));
	DescriptorHandleIncrementSize = GetParentLinkedDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	hCPUStart = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	hGPUStart = ShaderVisible ? pDescriptorHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE();

	IndexPool = { NumDescriptors };
}

void D3D12DynamicResourceDescriptorHeap::Allocate(
	D3D12_CPU_DESCRIPTOR_HANDLE& hCPU,
	D3D12_GPU_DESCRIPTOR_HANDLE& hGPU,
	UINT&						 Index)
{
	std::scoped_lock _(Mutex);

	Index = static_cast<UINT>(IndexPool.Allocate());
	hCPU  = this->hCPU(Index);
	hGPU  = this->hGPU(Index);
}

void D3D12DynamicResourceDescriptorHeap::Release(UINT Index)
{
	std::scoped_lock _(Mutex);

	IndexPool.Release(static_cast<size_t>(Index));
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DynamicResourceDescriptorHeap::hCPU(UINT Index) const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE hCPU = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(hCPU, hCPUStart, Index, DescriptorHandleIncrementSize);
	return hCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DynamicResourceDescriptorHeap::hGPU(UINT Index) const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE::InitOffsetted(hGPU, hGPUStart, Index, DescriptorHandleIncrementSize);
	return hGPU;
}

void D3D12DescriptorPage::Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	Desc = { .Type = Type, .NumDescriptors = NumDescriptors, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 };

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pDescriptorHeap)));
	DescriptorHandleIncrementSize = GetParentLinkedDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	hCPUStart = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

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
	TSize					   Size = iter->first;
	FreeBlocksByOffsetPoolIter OffsetIter{ iter->second };
	TOffset					   Offset = OffsetIter.Iterator->first;

	// Remove the block from the pool
	SizePool.erase(Size);
	FreeBlocksByOffsetPool.erase(OffsetIter.Iterator);

	// Allocate a available block
	TOffset NewOffset = Offset + NumDescriptors;
	TSize	NewSize	  = Size - NumDescriptors;

	// Return left over block to the pool
	if (NewSize > 0)
	{
		AllocateFreeBlock(NewOffset, NewSize);
	}

	Desc.NumDescriptors -= NumDescriptors;

	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(hCPUStart, Offset, DescriptorHandleIncrementSize);
	return D3D12DescriptorArray(this, Handle, Offset, NumDescriptors);
}

void D3D12DescriptorPage::Release(D3D12DescriptorArray&& DescriptorArray)
{
	FreeBlock(DescriptorArray.GetOffset(), DescriptorArray.GetNumDescriptors());
}

void D3D12DescriptorPage::AllocateFreeBlock(TOffset Offset, TSize Size)
{
	auto OffsetElement					 = FreeBlocksByOffsetPool.emplace(Offset, SizePool.begin());
	auto SizeElement					 = SizePool.emplace(Size, OffsetElement.first);
	OffsetElement.first->second.Iterator = SizeElement;
}

void D3D12DescriptorPage::FreeBlock(TOffset Offset, TSize Size)
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
		TOffset PrevOffset = PrevIter->first;
		TSize	PrevSize   = PrevIter->second.Iterator->first;
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
		TOffset NextOffset = NextIter->first;
		TSize	NextSize   = NextIter->second.Iterator->first;
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
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorArray::operator[](UINT Index) const noexcept
{
	assert(Index >= 0 && Index < NumDescriptors);
	CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(CPUDescriptorHandle);
	return Handle.Offset(Index, Parent->GetDescriptorHandleIncrementSize());
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
		auto NewPage = std::make_unique<D3D12DescriptorPage>(GetParentLinkedDevice());
		NewPage->Initialize(CVar_DescriptorPageSize, Type);

		DescriptorArray = NewPage->Allocate(NumDescriptors).value();
		DescriptorPages.push_back(std::move(NewPage));
	}

	return DescriptorArray;
}

void D3D12DynamicDescriptorHeap::Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	Desc = { .Type			 = Type,
			 .NumDescriptors = NumDescriptors,
			 .Flags			 = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			 .NodeMask		 = 0 };

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pDescriptorHeap)));
	DescriptorHandleIncrementSize = GetParentLinkedDevice()->GetDevice()->GetDescriptorHandleIncrementSize(Type);

	GraphicsHandleCache.Initialize(Type, DescriptorHandleIncrementSize);
	ComputeHandleCache.Initialize(Type, DescriptorHandleIncrementSize);

	hCPU = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	hGPU = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

void D3D12DynamicDescriptorHeap::Reset()
{
	hCPU = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	hGPU = pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	GraphicsHandleCache.Reset();
	ComputeHandleCache.Reset();
}

UINT D3D12DescriptorHandleCache::CommitDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE& hDestCPU,
	CD3DX12_GPU_DESCRIPTOR_HANDLE& hDestGPU,
	ID3D12GraphicsCommandList*	   pCommandList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*pfnSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	UINT NumStaleDescriptorTables = 0;
	UINT RootIndices[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT];

	for (size_t i = 0; i < StaleDescriptorTableBitMask.size(); ++i)
	{
		if (StaleDescriptorTableBitMask.test(i))
		{
			RootIndices[i] = static_cast<UINT>(i);

			NumStaleDescriptorTables++;
		}
	}

	UINT NumDescriptorsCommitted = 0;
	UINT RootParameterIndex		 = 0;
	for (UINT i = 0; i < NumStaleDescriptorTables; ++i)
	{
		RootParameterIndex							   = RootIndices[i];
		UINT						 NumSrcDescriptors = DescriptorTableCaches[RootParameterIndex].NumDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE* pSrcHandle		   = DescriptorTableCaches[RootParameterIndex].BaseDescriptor;

		GetParentLinkedDevice()
			->GetDevice()
			->CopyDescriptors(1, &hDestCPU, &NumSrcDescriptors, NumSrcDescriptors, pSrcHandle, nullptr, Type);

		(pCommandList->*pfnSetFunc)(RootParameterIndex, hDestGPU);

		// Offset current CPU and GPU descriptor handles.
		hDestCPU.Offset(NumSrcDescriptors, DescriptorHandleIncrementSize);
		hDestGPU.Offset(NumSrcDescriptors, DescriptorHandleIncrementSize);
		NumDescriptorsCommitted += NumSrcDescriptors;

		// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new
		// descriptor.
		StaleDescriptorTableBitMask.flip(i);
	}

	return NumDescriptorsCommitted;
}
