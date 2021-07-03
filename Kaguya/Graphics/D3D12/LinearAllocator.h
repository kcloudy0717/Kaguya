#pragma once
#include <Core/CriticalSection.h>

struct Allocation
{
	ID3D12Resource*			  pResource;
	UINT64					  Offset;
	UINT64					  Size;
	BYTE*					  CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
};

class LinearAllocatorPage
{
public:
	LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, UINT64 PageSize)
		: pResource(pResource)
		, Offset(0)
		, PageSize(PageSize)
	{
		pResource->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress));
		GPUVirtualAddress = pResource->GetGPUVirtualAddress();
	}

	~LinearAllocatorPage() { pResource->Unmap(0, nullptr); }

	std::optional<Allocation> Suballocate(UINT64 Size, UINT Alignment);

	void Reset();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	UINT64								   Offset;
	UINT64								   PageSize;
	BYTE*								   CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS			   GPUVirtualAddress;
};

class LinearAllocator
{
public:
	enum
	{
		DefaultAlignment = 256,

		CpuAllocatorPageSize = 0x200000, // 2MB
	};

	LinearAllocator(ID3D12Device* Device);

	void Begin(UINT64 CompletedFenceValue);

	void End(UINT64 FenceValue);

	Allocation Allocate(UINT64 Size, UINT Alignment = DefaultAlignment);

private:
	LinearAllocatorPage* RequestPage(UINT64 CompletedFenceValue);

	LinearAllocatorPage* CreateNewPage(UINT64 PageSize);

	void DiscardPages(UINT64 FenceValue, const std::vector<LinearAllocatorPage*>& Pages);

private:
	ID3D12Device* Device;

	std::vector<std::unique_ptr<LinearAllocatorPage>>	PagePool;
	std::queue<std::pair<UINT64, LinearAllocatorPage*>> RetiredPages;
	std::queue<LinearAllocatorPage*>					AvailablePages;
	CriticalSection										CriticalSection;

	UINT64							  CompletedFenceValue;
	LinearAllocatorPage*			  CurrentPage;
	std::vector<LinearAllocatorPage*> RetiredPageList;
};
