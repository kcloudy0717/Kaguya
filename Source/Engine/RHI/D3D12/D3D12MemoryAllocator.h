#pragma once
#include "D3D12Core.h"

struct D3D12Allocation
{
	ID3D12Resource*			  Resource;
	UINT64					  Offset;
	UINT64					  Size;
	BYTE*					  CpuVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
};

class D3D12LinearAllocatorPage
{
public:
	explicit D3D12LinearAllocatorPage(
		ARC<ID3D12Resource> Resource,
		UINT64				PageSize);
	~D3D12LinearAllocatorPage();

	std::optional<D3D12Allocation> Suballocate(UINT64 Size, UINT Alignment);

	void Reset();

private:
	ARC<ID3D12Resource>		  Resource;
	UINT64					  Offset;
	UINT64					  PageSize;
	BYTE*					  CpuVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
};

class D3D12LinearAllocator : public D3D12LinkedDeviceChild
{
public:
	static constexpr UINT64 CpuAllocatorPageSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	explicit D3D12LinearAllocator(D3D12LinkedDevice* Parent)
		: D3D12LinkedDeviceChild(Parent)
	{
	}

	// Versions all the current constant data with SyncHandle to ensure memory is not overriden when it GPU uses it
	void Version(D3D12SyncHandle SyncHandle);

	[[nodiscard]] D3D12Allocation Allocate(
		UINT64 Size,
		UINT   Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	template<typename T>
	[[nodiscard]] D3D12Allocation Allocate(
		const T& Data,
		UINT	 Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
	{
		D3D12Allocation Allocation = Allocate(sizeof(T), Alignment);
		std::memcpy(Allocation.CpuVirtualAddress, &Data, sizeof(T));
		return Allocation;
	}

private:
	[[nodiscard]] D3D12LinearAllocatorPage* RequestPage();

	[[nodiscard]] std::unique_ptr<D3D12LinearAllocatorPage> CreateNewPage(UINT64 PageSize) const;

	void DiscardPages(UINT64 FenceValue, const std::vector<D3D12LinearAllocatorPage*>& Pages);

private:
	std::vector<std::unique_ptr<D3D12LinearAllocatorPage>>	 PagePool;
	std::queue<std::pair<UINT64, D3D12LinearAllocatorPage*>> RetiredPages;
	std::queue<D3D12LinearAllocatorPage*>					 AvailablePages;

	D3D12SyncHandle						   SyncHandle;
	D3D12LinearAllocatorPage*			   CurrentPage = nullptr;
	std::vector<D3D12LinearAllocatorPage*> RetiredPageList;
};
