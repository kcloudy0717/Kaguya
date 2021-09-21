#include "D3D12MemoryAllocator.h"
#include "D3D12LinkedDevice.h"

std::optional<D3D12Allocation> D3D12LinearAllocatorPage::Suballocate(UINT64 Size, UINT Alignment)
{
	UINT64 AlignedSize = AlignUp(Size, static_cast<UINT64>(Alignment));
	if (Offset + AlignedSize > this->PageSize)
	{
		return {};
	}

	D3D12Allocation Allocation = { .pResource		  = pResource.Get(),
								   .Offset			  = Offset,
								   .Size			  = Size,
								   .CPUVirtualAddress = CPUVirtualAddress + Offset,
								   .GPUVirtualAddress = GPUVirtualAddress + Offset };

	Offset += AlignedSize;

	return Allocation;
}

void D3D12LinearAllocatorPage::Reset()
{
	Offset = 0;
}

void D3D12LinearAllocator::Version(D3D12CommandSyncPoint SyncPoint)
{
	if (!CurrentPage)
	{
		return;
	}

	this->SyncPoint = SyncPoint;

	RetiredPageList.push_back(CurrentPage);
	CurrentPage = nullptr;

	DiscardPages(SyncPoint.GetValue(), RetiredPageList);
	RetiredPageList.clear();
}

D3D12Allocation D3D12LinearAllocator::Allocate(UINT64 Size, UINT Alignment /*= DefaultAlignment*/)
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

	while (SyncPoint.IsValid() && !RetiredPages.empty() && RetiredPages.front().first <= SyncPoint.GetValue())
	{
		AvailablePages.push(RetiredPages.front().second);
		RetiredPages.pop();
	}

	D3D12LinearAllocatorPage* pPage = nullptr;

	if (!AvailablePages.empty())
	{
		pPage = AvailablePages.front();
		pPage->Reset();
		AvailablePages.pop();
	}
	else
	{
		pPage = CreateNewPage(CpuAllocatorPageSize);
		PagePool.emplace_back(pPage);
	}

	return pPage;
}

D3D12LinearAllocatorPage* D3D12LinearAllocator::CreateNewPage(UINT64 PageSize)
{
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto ResourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

#ifdef _DEBUG
	pResource->SetName(L"Linear Allocator Page");
#endif

	return new D3D12LinearAllocatorPage(pResource, PageSize);
}

void D3D12LinearAllocator::DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages)
{
	std::scoped_lock _(CriticalSection);
	for (const auto& Page : Pages)
	{
		RetiredPages.push(std::make_pair(FenceValue, Page));
	}
}

D3D12_RESOURCE_STATES GetBufferInitialState(D3D12_HEAP_TYPE HeapType)
{
	switch (HeapType)
	{
	case D3D12_HEAP_TYPE_DEFAULT:
		return D3D12_RESOURCE_STATE_COMMON;
	case D3D12_HEAP_TYPE_UPLOAD:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	case D3D12_HEAP_TYPE_READBACK:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	default:
		assert("Should not get here" && false);
		return D3D12_RESOURCE_STATE_COMMON;
	}
}
