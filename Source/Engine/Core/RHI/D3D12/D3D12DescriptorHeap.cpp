#include "D3D12DescriptorHeap.h"
#include "D3D12LinkedDevice.h"

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
	std::scoped_lock Guard(Mutex);

	Index				= static_cast<UINT>(IndexPool.Allocate());
	CpuDescriptorHandle = this->GetCpuDescriptorHandle(Index);
	GpuDescriptorHandle = this->GetGpuDescriptorHandle(Index);
}

void D3D12DescriptorHeap::Release(UINT Index)
{
	std::scoped_lock Guard(Mutex);

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

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12DescriptorHeap::InitializeDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT					   NumDescriptors)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc = {};
	Desc.Type										  = Type;
	Desc.NumDescriptors								  = NumDescriptors;
	Desc.Flags										  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Desc.NodeMask									  = 0;
	VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(
		&Desc,
		IID_PPV_ARGS(&DescriptorHeap)));
	return DescriptorHeap;
}

D3D12DescriptorArray::D3D12DescriptorArray(
	D3D12DescriptorPage*		Parent,
	D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle,
	UINT						Offset,
	UINT						NumDescriptors) noexcept
	: Parent(Parent)
	, CpuDescriptorHandle(CpuDescriptorHandle)
	, Offset(Offset)
	, NumDescriptors(NumDescriptors)
{
}

D3D12DescriptorArray::D3D12DescriptorArray(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept
	: Parent(std::exchange(D3D12DescriptorArray.Parent, {}))
	, CpuDescriptorHandle(std::exchange(D3D12DescriptorArray.CpuDescriptorHandle, {}))
	, Offset(std::exchange(D3D12DescriptorArray.Offset, {}))
	, NumDescriptors(std::exchange(D3D12DescriptorArray.NumDescriptors, {}))
{
}

D3D12DescriptorArray& D3D12DescriptorArray::operator=(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept
{
	if (this == &D3D12DescriptorArray)
	{
		return *this;
	}

	// Release any descriptors if we have any
	InternalDestruct();
	Parent				= std::exchange(D3D12DescriptorArray.Parent, {});
	CpuDescriptorHandle = std::exchange(D3D12DescriptorArray.CpuDescriptorHandle, {});
	Offset				= std::exchange(D3D12DescriptorArray.Offset, {});
	NumDescriptors		= std::exchange(D3D12DescriptorArray.NumDescriptors, {});

	return *this;
}

D3D12DescriptorArray::~D3D12DescriptorArray()
{
	InternalDestruct();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorArray::operator[](UINT Index) const noexcept
{
	assert(Index < NumDescriptors);
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(CpuDescriptorHandle, static_cast<INT>(Index), Parent->GetDescriptorSize());
}

void D3D12DescriptorArray::InternalDestruct()
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

D3D12DescriptorPage::D3D12DescriptorPage(
	D3D12LinkedDevice*		   Parent,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT					   NumDescriptors)
	: D3D12LinkedDeviceChild(Parent)
	, DescriptorHeap(InitializeDescriptorHeap(Type, NumDescriptors))
	, Desc(DescriptorHeap->GetDesc())
	, CpuBaseAddress(DescriptorHeap->GetCPUDescriptorHandleForHeapStart())
	, DescriptorSize(Parent->GetParentDevice()->GetSizeOfDescriptor(Type))
{
	AllocateFreeBlock(0, NumDescriptors);
}

std::optional<D3D12DescriptorArray> D3D12DescriptorPage::Allocate(UINT NumDescriptors)
{
	std::scoped_lock Guard(Mutex);
	if (NumDescriptors > Desc.NumDescriptors)
	{
		return std::nullopt;
	}

	// Get a block in the pool that is large enough to satisfy the required descriptor,
	// The function returns an iterator pointing to the key in the map container which is equivalent to k passed in the
	// parameter. In case k is not present in the map container, the function returns an iterator pointing to the
	// immediate next element which is just greater than k.
	auto Iter = SizePool.lower_bound(NumDescriptors);
	if (Iter == SizePool.end())
	{
		return std::nullopt;
	}

	// Query all information about this available block
	SizeType				   Size = Iter->first;
	FreeBlocksByOffsetPoolIter OffsetIter(Iter->second);
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
	std::scoped_lock Guard(Mutex);
	FreeBlock(DescriptorArray.GetOffset(), DescriptorArray.GetNumDescriptors());
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12DescriptorPage::InitializeDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT					   NumDescriptors)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc = {};
	Desc.Type										  = Type;
	Desc.NumDescriptors								  = NumDescriptors;
	Desc.Flags										  = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask									  = 0;
	VERIFY_D3D12_API(Parent->GetDevice()->CreateDescriptorHeap(
		&Desc,
		IID_PPV_ARGS(&DescriptorHeap)));
	return DescriptorHeap;
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

D3D12DescriptorArray D3D12DescriptorAllocator::Allocate(UINT NumDescriptors)
{
	D3D12DescriptorArray DescriptorArray;

	for (auto& Page : DescriptorPages)
	{
		auto OptDescriptorArray = Page->Allocate(NumDescriptors);
		if (OptDescriptorArray.has_value())
		{
			DescriptorArray = std::move(OptDescriptorArray.value());
			break;
		}
	}

	if (!DescriptorArray.IsValid())
	{
		auto& NewPage	= DescriptorPages.emplace_back(std::make_unique<D3D12DescriptorPage>(GetParentLinkedDevice(), Type, PageSize));
		DescriptorArray = NewPage->Allocate(NumDescriptors).value();
	}

	return DescriptorArray;
}
