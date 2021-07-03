#include "pch.h"
#include "LinearAllocator.h"
#include "d3dx12.h"

std::optional<Allocation> LinearAllocatorPage::Suballocate(UINT64 Size, UINT Alignment)
{
	UINT64 NewOffset = AlignUp(Offset, static_cast<UINT64>(Alignment));
	if (NewOffset + Size > this->Size)
	{
		return {};
	}

	Allocation Allocation = { .pResource		 = pResource.Get(),
							  .Offset			 = Offset,
							  .Size				 = Size,
							  .CPUVirtualAddress = CPUVirtualAddress + NewOffset,
							  .GPUVirtualAddress = GPUVirtualAddress + NewOffset };

	Offset = NewOffset + Size;

	return Allocation;
}

void LinearAllocatorPage::Reset()
{
	Offset = 0;
}

LinearAllocator::LinearAllocator(ID3D12Device* Device)
	: Device(Device)
	, CompletedFenceValue(0)
	, CurrentPage(nullptr)
{
}

void LinearAllocator::Begin(UINT64 CompletedFenceValue)
{
	this->CompletedFenceValue = CompletedFenceValue;
}

void LinearAllocator::End(UINT64 FenceValue)
{
	if (!CurrentPage)
	{
		return;
	}

	RetiredPageList.push_back(CurrentPage);
	CurrentPage = nullptr;

	DiscardPages(FenceValue, RetiredPageList);
	RetiredPageList.clear();
}

Allocation LinearAllocator::Allocate(UINT64 Size, UINT Alignment /*= DefaultAlignment*/)
{
	if (!CurrentPage)
	{
		CurrentPage = RequestPage(CompletedFenceValue);
	}

	std::optional<Allocation> OptAllocation = CurrentPage->Suballocate(Size, Alignment);

	if (!OptAllocation)
	{
		RetiredPageList.push_back(CurrentPage);

		CurrentPage	  = RequestPage(CompletedFenceValue);
		OptAllocation = CurrentPage->Suballocate(Size, Alignment);
		assert(OptAllocation.has_value());
	}

	return OptAllocation.value();
}

LinearAllocatorPage* LinearAllocator::RequestPage(UINT64 CompletedFenceValue)
{
	std::scoped_lock _(CriticalSection);

	while (!RetiredPages.empty() && RetiredPages.front().first <= CompletedFenceValue)
	{
		AvailablePages.push(RetiredPages.front().second);
		RetiredPages.pop();
	}

	LinearAllocatorPage* pPage = nullptr;

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

LinearAllocatorPage* LinearAllocator::CreateNewPage(UINT64 PageSize)
{
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto ResourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf()));

#ifdef _DEBUG
	pResource->SetName(L"Linear Allocator Page");
#endif

	return new LinearAllocatorPage(pResource, PageSize);
}

void LinearAllocator::DiscardPages(UINT64 FenceValue, const std::vector<LinearAllocatorPage*>& Pages)
{
	std::scoped_lock _(CriticalSection);
	for (const auto& Page : Pages)
	{
		RetiredPages.push(std::make_pair(FenceValue, Page));
	}
}
