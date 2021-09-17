#pragma once
#include <Core/CoreDefines.h>
#include <Core/CriticalSection.h>
#include "D3D12Common.h"

struct D3D12Allocation
{
	ID3D12Resource*			  pResource;
	UINT64					  Offset;
	UINT64					  Size;
	BYTE*					  CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
};

class D3D12LinearAllocatorPage
{
public:
	D3D12LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, UINT64 PageSize)
		: pResource(pResource)
		, Offset(0)
		, PageSize(PageSize)
	{
		pResource->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress));
		GPUVirtualAddress = pResource->GetGPUVirtualAddress();
	}

	~D3D12LinearAllocatorPage() { pResource->Unmap(0, nullptr); }

	std::optional<D3D12Allocation> Suballocate(UINT64 Size, UINT Alignment);

	void Reset();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	UINT64								   Offset;
	UINT64								   PageSize;
	BYTE*								   CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS			   GPUVirtualAddress;
};

class D3D12LinearAllocator : public D3D12LinkedDeviceChild
{
public:
	enum
	{
		CpuAllocatorPageSize = 64 * 1024
	};

	D3D12LinearAllocator(D3D12LinkedDevice* Parent)
		: D3D12LinkedDeviceChild(Parent)
	{
	}

	// Versions all the current constant data with SyncPoint to ensure memory is not overriden when it GPU uses it
	void Version(D3D12CommandSyncPoint SyncPoint);

	D3D12Allocation Allocate(UINT64 Size, UINT Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	template<typename T>
	D3D12Allocation Allocate(const T& Data, UINT Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
	{
		D3D12Allocation Allocation = Allocate(sizeof(T), Alignment);
		std::memcpy(Allocation.CPUVirtualAddress, &Data, sizeof(T));
		return Allocation;
	}

private:
	D3D12LinearAllocatorPage* RequestPage();

	D3D12LinearAllocatorPage* CreateNewPage(UINT64 PageSize);

	void DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages);

private:
	std::vector<std::unique_ptr<D3D12LinearAllocatorPage>>	 PagePool;
	std::queue<std::pair<UINT64, D3D12LinearAllocatorPage*>> RetiredPages;
	std::queue<D3D12LinearAllocatorPage*>					 AvailablePages;
	CriticalSection											 CriticalSection;

	D3D12CommandSyncPoint				   SyncPoint;
	D3D12LinearAllocatorPage*			   CurrentPage = nullptr;
	std::vector<D3D12LinearAllocatorPage*> RetiredPageList;
};
