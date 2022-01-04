#pragma once
#include "D3D12Common.h"
#include "D3D12LinkedDevice.h"
#include "D3D12Resource.h"

class D3D12RaytracingGeometry
{
public:
	D3D12RaytracingGeometry() noexcept = default;
	explicit D3D12RaytracingGeometry(size_t Size)
	{
		RaytracingGeometryDescs.reserve(Size);
	}

	auto begin() noexcept { return RaytracingGeometryDescs.begin(); }
	auto end() noexcept { return RaytracingGeometryDescs.end(); }

	[[nodiscard]] UINT Size() const noexcept { return static_cast<UINT>(RaytracingGeometryDescs.size()); }

	[[nodiscard]] D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS GetInputsDesc() const
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Desc = {};
		Desc.Type												  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		Desc.Flags												  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
					 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		Desc.NumDescs		= Size();
		Desc.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY;
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
	explicit D3D12RaytracingScene(size_t Size)
	{
		RaytracingInstanceDescs.reserve(Size);
	}

	auto begin() noexcept { return RaytracingInstanceDescs.begin(); }
	auto end() noexcept { return RaytracingInstanceDescs.end(); }

	[[nodiscard]] size_t size() const noexcept { return RaytracingInstanceDescs.size(); }
	[[nodiscard]] bool	 empty() const noexcept { return RaytracingInstanceDescs.empty(); }

	void Reset() noexcept;

	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc);

	void ComputeMemoryRequirements(ID3D12Device5* Device, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);

	void Generate(
		ID3D12GraphicsCommandList4* CommandList,
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
	explicit D3D12RaytracingMemoryPage(
		D3D12LinkedDevice*	  Parent,
		UINT64				  PageSize,
		D3D12_HEAP_TYPE		  HeapType,
		D3D12_RESOURCE_STATES InitialResourceState);

	[[nodiscard]] ID3D12Resource*			GetResource() const noexcept { return Resource.Get(); }
	[[nodiscard]] UINT64					GetPageSize() const noexcept { return PageSize; }
	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const noexcept { return VirtualAddress; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> InitializeResource(
		UINT64				  PageSize,
		D3D12_HEAP_TYPE		  HeapType,
		D3D12_RESOURCE_STATES InitialResourceState);

private:
	friend class D3D12RaytracingMemoryAllocator;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	UINT64								   PageSize		  = 0;
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
	D3D12SyncHandle			SyncHandle;
};

class D3D12RaytracingAccelerationStructureManager : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingAccelerationStructureManager() noexcept = default;
	explicit D3D12RaytracingAccelerationStructureManager(
		D3D12LinkedDevice* Parent,
		UINT64			   PageSize);

	UINT64 Build(
		ID3D12GraphicsCommandList4*									CommandList,
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs);

	void Copy(
		ID3D12GraphicsCommandList4* CommandList);

	void Compact(
		ID3D12GraphicsCommandList4* CommandList,
		UINT64						AccelerationStructureIndex);

	void SetSyncHandle(
		UINT64			AccelerationStructureIndex,
		D3D12SyncHandle SyncHandle);

	D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress(
		UINT64 AccelerationStructureIndex);

private:
	UINT64 GetAccelerationStructureIndex();

	void ReleaseAccelerationStructure(UINT64 Index);

private:
	UINT64 PageSize = 0;

	UINT64 TotalUncompactedMemory = 0;
	UINT64 TotalCompactedMemory	  = 0;

	std::vector<D3D12AccelerationStructure> AccelerationStructures;
	std::queue<UINT64>						IndexQueue;
	UINT64									Index = 0;

	D3D12RaytracingMemoryAllocator ScratchPool;
	D3D12RaytracingMemoryAllocator ResultPool;
	D3D12RaytracingMemoryAllocator ResultCompactedPool;
	D3D12RaytracingMemoryAllocator CompactedSizeGpuPool;
	D3D12RaytracingMemoryAllocator CompactedSizeCpuPool;
};

// ========== Miss shader table indexing ==========
// Simple pointer arithmetic
// MissShaderRecordAddress = D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StartAddress +
// D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StrideInBytes * MissShaderIndex
//

//
// ========== Hit group table indexing ==========
// HitGroupRecordAddress = D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StartAddress +
// D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StrideInBytes * HitGroupEntryIndex
//
// where HitGroupEntryIndex =
// (RayContributionToHitGroupIndex + (MultiplierForGeometryContributionToHitGroupIndex *
// GeometryContributionToHitGroupIndex) + D3D12_RAYTRACING_INSTANCE_DESC.InstanceContributionToHitGroupIndex)
//
//	GeometryContributionToHitGroupIndex is a system generated index of geometry in BLAS (0,1,2,3..)
//

// This blog post by Will Usher is incredibly useful for calculating sbt in various graphics APIs
// https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways

struct ShaderIdentifier
{
	ShaderIdentifier() noexcept = default;
	ShaderIdentifier(void* Data)
	{
		std::memcpy(this->Data, Data, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}

	BYTE Data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

// RaytracingShaderRecord = {{Shader Identifier}, {RootArguments}}
template<typename T>
struct D3D12RaytracingShaderRecord
{
	static_assert(std::is_trivially_copyable_v<T>, "typename T must be trivially copyable");

	D3D12RaytracingShaderRecord() noexcept = default;
	D3D12RaytracingShaderRecord(const ShaderIdentifier& ShaderIdentifier, const T& RootArguments) noexcept
		: ShaderIdentifier(ShaderIdentifier)
		, RootArguments(RootArguments)
	{
	}

	ShaderIdentifier ShaderIdentifier;
	T				 RootArguments;
};

template<>
struct D3D12RaytracingShaderRecord<void>
{
	D3D12RaytracingShaderRecord() noexcept = default;
	D3D12RaytracingShaderRecord(const ShaderIdentifier& ShaderIdentifier) noexcept
		: ShaderIdentifier(ShaderIdentifier)
	{
	}

	ShaderIdentifier ShaderIdentifier;
};

class IRaytracingShaderTable
{
public:
	virtual ~IRaytracingShaderTable() = default;

	UINT GetNumShaderRecords() const { return NumShaderRecords; }

	virtual UINT64 GetTotalSizeInBytes() const = 0;
	virtual UINT64 GetSizeInBytes() const	   = 0;
	virtual UINT64 GetStrideInBytes() const	   = 0;

	virtual void Write(BYTE* Dst) const = 0;

protected:
	UINT NumShaderRecords = 0;
};

template<typename T>
class D3D12RaytracingShaderTable : public IRaytracingShaderTable
{
public:
	using Record = D3D12RaytracingShaderRecord<T>;
	static constexpr UINT64 StrideInBytes =
		AlignUp<UINT64>(sizeof(Record), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	D3D12RaytracingShaderTable(size_t NumShaderRecords)
		: ShaderRecords(NumShaderRecords)
	{
	}

	// Accounts for all shader records
	UINT64 GetTotalSizeInBytes() const override
	{
		UINT64 SizeInBytes = static_cast<UINT64>(ShaderRecords.size()) * GetStrideInBytes();
		SizeInBytes		   = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		return SizeInBytes;
	}

	// Accounts for filled shader records
	UINT64 GetSizeInBytes() const override
	{
		UINT64 SizeInBytes = static_cast<UINT64>(NumShaderRecords) * GetStrideInBytes();
		SizeInBytes		   = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		return SizeInBytes;
	}

	UINT64 GetStrideInBytes() const override { return StrideInBytes; }

	void Write(BYTE* Dst) const override
	{
		for (const auto& Record : ShaderRecords)
		{
			// Copy record data
			memcpy(Dst, &Record, sizeof(Record));

			Dst += StrideInBytes;
		}
	}

	void Reset() { NumShaderRecords = 0; }

	void AddShaderRecord(const Record& Record) { ShaderRecords[NumShaderRecords++] = Record; }

private:
	std::vector<Record> ShaderRecords;
};

// Collection of RaytracingShaderTables
class D3D12RaytracingShaderBindingTable
{
public:
	template<typename T>
	D3D12RaytracingShaderTable<T>* AddRayGenerationShaderTable(UINT NumRayGenerationShaders)
	{
		assert(RayGenerationShaderTable == nullptr);
		auto Table				 = new D3D12RaytracingShaderTable<T>(NumRayGenerationShaders);
		RayGenerationShaderTable = std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	template<typename T>
	D3D12RaytracingShaderTable<T>* AddMissShaderTable(UINT NumMissShaders)
	{
		assert(MissShaderTable == nullptr);
		auto Table		= new D3D12RaytracingShaderTable<T>(NumMissShaders);
		MissShaderTable = std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	template<typename T>
	D3D12RaytracingShaderTable<T>* AddHitGroupShaderTable(UINT NumHitGroups)
	{
		assert(HitGroupShaderTable == nullptr);
		auto Table			= new D3D12RaytracingShaderTable<T>(NumHitGroups);
		HitGroupShaderTable = std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	// Call this after shader tables are added
	void Generate(D3D12LinkedDevice* Device);

	// Call this after shader records for the tables have been filled out to upload the records to GPU table
	void WriteToGpu(ID3D12GraphicsCommandList* CommandList) const;

	[[nodiscard]] D3D12_DISPATCH_RAYS_DESC GetDesc(UINT RayGenerationShaderIndex, UINT BaseMissShaderIndex) const;

private:
	std::unique_ptr<IRaytracingShaderTable> RayGenerationShaderTable;
	std::unique_ptr<IRaytracingShaderTable> MissShaderTable;
	std::unique_ptr<IRaytracingShaderTable> HitGroupShaderTable;

	UINT64 RayGenerationShaderTableOffset = 0;
	UINT64 MissShaderTableOffset		  = 0;
	UINT64 HitGroupShaderTableOffset	  = 0;

	UINT64 SizeInBytes = 0;

	D3D12Buffer				SBTBuffer, SBTUploadBuffer;
	std::unique_ptr<BYTE[]> CpuData;
};
