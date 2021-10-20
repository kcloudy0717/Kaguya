#include "D3D12MemoryAllocator.h"
#include "D3D12LinkedDevice.h"

std::optional<D3D12Allocation> D3D12LinearAllocatorPage::Suballocate(UINT64 Size, UINT Alignment)
{
	UINT64 AlignedSize = AlignUp(Size, static_cast<UINT64>(Alignment));
	if (Offset + AlignedSize > this->PageSize)
	{
		return {};
	}

	D3D12Allocation Allocation = { .Resource		  = Resource.Get(),
								   .Offset			  = Offset,
								   .Size			  = Size,
								   .CpuVirtualAddress = CpuVirtualAddress + Offset,
								   .GpuVirtualAddress = GpuVirtualAddress + Offset };

	Offset += AlignedSize;

	return Allocation;
}

void D3D12LinearAllocatorPage::Reset()
{
	Offset = 0;
}

void D3D12LinearAllocator::Version(D3D12SyncHandle SyncHandle)
{
	if (!CurrentPage)
	{
		return;
	}

	this->SyncHandle = SyncHandle;

	RetiredPageList.push_back(CurrentPage);
	CurrentPage = nullptr;

	DiscardPages(SyncHandle.GetValue(), RetiredPageList);
	RetiredPageList.clear();
}

D3D12Allocation D3D12LinearAllocator::Allocate(
	UINT64 Size,
	UINT   Alignment /*= D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT*/)
{
	if (!CurrentPage)
	{
		CurrentPage = RequestPage();
	}

	std::optional<D3D12Allocation> OptAllocation = CurrentPage->Suballocate(Size, Alignment);

	if (!OptAllocation)
	{
		RetiredPageList.push_back(CurrentPage);

		CurrentPage	  = RequestPage();
		OptAllocation = CurrentPage->Suballocate(Size, Alignment);
		assert(OptAllocation.has_value());
	}

	return OptAllocation.value();
}

D3D12LinearAllocatorPage* D3D12LinearAllocator::RequestPage()
{
	std::scoped_lock _(CriticalSection);

	while (SyncHandle.IsValid() && !RetiredPages.empty() && RetiredPages.front().first <= SyncHandle.GetValue())
	{
		AvailablePages.push(RetiredPages.front().second);
		RetiredPages.pop();
	}

	D3D12LinearAllocatorPage* Page = nullptr;

	if (!AvailablePages.empty())
	{
		Page = AvailablePages.front();
		Page->Reset();
		AvailablePages.pop();
	}
	else
	{
		Page = CreateNewPage(CpuAllocatorPageSize);
		PagePool.emplace_back(Page);
	}

	return Page;
}

D3D12LinearAllocatorPage* D3D12LinearAllocator::CreateNewPage(UINT64 PageSize) const
{
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto ResourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));

#ifdef _DEBUG
	Resource->SetName(L"Linear Allocator Page");
#endif

	return new D3D12LinearAllocatorPage(Resource, PageSize);
}

void D3D12LinearAllocator::DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages)
{
	std::scoped_lock _(CriticalSection);
	for (const auto& Page : Pages)
	{
		RetiredPages.push(std::make_pair(FenceValue, Page));
	}
}
