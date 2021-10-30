#pragma once
#include "D3D12Common.h"

class D3D12RaytracingGeometry
{
public:
	D3D12RaytracingGeometry() noexcept = default;
	explicit D3D12RaytracingGeometry(size_t Size) { RaytracingGeometryDescs.reserve(Size); }

	auto begin() noexcept { return RaytracingGeometryDescs.begin(); }
	auto end() noexcept { return RaytracingGeometryDescs.end(); }

	[[nodiscard]] UINT Size() const noexcept { return static_cast<UINT>(RaytracingGeometryDescs.size()); }

	[[nodiscard]] D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS GetInputsDesc() const
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Desc = {};
		Desc.Type  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		Desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
					 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		Desc.NumDescs = Size(), Desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		Desc.pGeometryDescs = RaytracingGeometryDescs.data();
		return Desc;
	}

	void AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc);

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> RaytracingGeometryDescs;
};

// https://developer.nvidia.com/blog/rtx-best-practices/
// We should rebuild the TLAS rather than update, It’s just easier to manage in most circumstances, and the cost savings
// to refit likely aren’t worth sacrificing quality of TLAS.
class D3D12RaytracingScene
{
public:
	D3D12RaytracingScene() noexcept = default;
	explicit D3D12RaytracingScene(size_t Size) { RaytracingInstanceDescs.reserve(Size); }

	auto begin() noexcept { return RaytracingInstanceDescs.begin(); }
	auto end() noexcept { return RaytracingInstanceDescs.end(); }

	[[nodiscard]] size_t size() const noexcept { return RaytracingInstanceDescs.size(); }
	[[nodiscard]] bool	 empty() const noexcept { return RaytracingInstanceDescs.empty(); }

	void Reset() noexcept;

	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc);

	void ComputeMemoryRequirements(ID3D12Device5* Device, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);

	void Generate(
		ID3D12GraphicsCommandList6* CommandList,
		ID3D12Resource*				Scratch,
		ID3D12Resource*				Result,
		D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs);

private:
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> RaytracingInstanceDescs;
	UINT64										ScratchSizeInBytes = 0;
	UINT64										ResultSizeInBytes  = 0;
};

// https://github.com/NVIDIAGameWorks/RTXMU, RTXMU is licensed under the MIT License.
// Modified for my own use

class D3D12RaytracingMemoryPage;

struct RaytracingMemorySection
{
	D3D12RaytracingMemoryPage* Parent		  = nullptr;
	UINT64					   Size			  = 0;
	UINT64					   Offset		  = 0;
	UINT64					   UnusedSize	  = 0;
	D3D12_GPU_VIRTUAL_ADDRESS  VirtualAddress = {};
	bool					   IsFree		  = false;
};

class D3D12RaytracingMemoryPage : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingMemoryPage() noexcept = default;
	D3D12RaytracingMemoryPage(D3D12LinkedDevice* Parent, UINT64 PageSize)
		: D3D12LinkedDeviceChild(Parent)
		, PageSize(PageSize)
	{
	}

	void Initialize(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialResourceState);

	UINT64 PageSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12_GPU_VIRTUAL_ADDRESS			   VirtualAddress = {};

	std::vector<RaytracingMemorySection> FreeMemorySections;
	UINT64								 CurrentOffset = 0;
	UINT64								 NumSubBlocks  = 0;
};

class D3D12RaytracingMemoryAllocator : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingMemoryAllocator() noexcept = default;
	D3D12RaytracingMemoryAllocator(
		D3D12LinkedDevice*	  Parent,
		D3D12_HEAP_TYPE		  HeapType,
		D3D12_RESOURCE_STATES InitialResourceState,
		UINT64				  DefaultPageSize,
		UINT64				  Alignment);

	auto& GetPages() { return Pages; }

	RaytracingMemorySection Allocate(UINT64 SizeInBytes);

	void Release(RaytracingMemorySection* Section);

	UINT64 GetSize();

private:
	void CreatePage(UINT64 PageSize);

	bool FindFreeSubBlock(
		D3D12RaytracingMemoryPage* Page,
		RaytracingMemorySection*   OutMemorySection,
		UINT64					   SizeInBytes);

	D3D12_HEAP_TYPE		  HeapType;
	D3D12_RESOURCE_STATES InitialResourceState;
	UINT64				  DefaultPageSize;
	UINT64				  Alignment;

	std::vector<std::unique_ptr<D3D12RaytracingMemoryPage>> Pages;
};

struct D3D12AccelerationStructure
{
	UINT64					CompactedSizeInBytes = 0;
	UINT64					ResultSize			 = 0;
	bool					IsCompacted			 = false;
	bool					RequestedCompaction	 = false;
	bool					ReadyToFree			 = false;
	RaytracingMemorySection ScratchMemory;
	RaytracingMemorySection ResultMemory;
	RaytracingMemorySection ResultCompactedMemory;
	RaytracingMemorySection CompactedSizeCpuMemory;
	RaytracingMemorySection CompactedSizeGpuMemory;

	D3D12SyncHandle SyncHandle;
};

class D3D12RaytracingAccelerationStructureManager : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingAccelerationStructureManager() noexcept = default;
	D3D12RaytracingAccelerationStructureManager(D3D12LinkedDevice* Parent, UINT64 PageSize);

	UINT64 Build(
		ID3D12GraphicsCommandList4*									CommandList,
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs);

	void Copy(ID3D12GraphicsCommandList4* CommandList);

	// Returns true if compacted
	void Compact(ID3D12GraphicsCommandList4* CommandList, UINT64 AccelerationStructureIndex);

	void SetSyncHandle(UINT64 AccelerationStructureIndex, D3D12SyncHandle SyncHandle);

	D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress(UINT64 AccelerationStructureIndex);

private:
	UINT64 GetAccelerationStructureIndex();

	void ReleaseAccelerationStructure(UINT64 Index);

private:
	UINT64 PageSize = 0;

	UINT64 TotalUncompactedMemory = 0;
	UINT64 TotalCompactedMemory	  = 0;

	std::vector<std::unique_ptr<D3D12AccelerationStructure>> AccelerationStructures;
	std::queue<UINT64>										 IndexQueue;
	UINT64													 Index = 0;

	D3D12RaytracingMemoryAllocator ScratchPool;
	D3D12RaytracingMemoryAllocator ResultPool;
	D3D12RaytracingMemoryAllocator ResultCompactedPool;
	D3D12RaytracingMemoryAllocator CompactedSizeGpuPool;
	D3D12RaytracingMemoryAllocator CompactedSizeCpuPool;
};
