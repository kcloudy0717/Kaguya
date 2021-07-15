#pragma once
#include <vector>
#include <Core/CoreDefines.h>
#include "Device.h"

// 	========== Miss shader table indexing ==========
// 	Simple pointer arithmetic
// 	MissShaderRecordAddress = D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StartAddress +
// D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StrideInBytes * MissShaderIndex
//

//
//	========== Hit group table indexing ==========
//	HitGroupRecordAddress = D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StartAddress +
// D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StrideInBytes * HitGroupEntryIndex
//
//	where HitGroupEntryIndex =
//	(RayContributionToHitGroupIndex + (MultiplierForGeometryContributionToHitGroupIndex *
// GeometryContributionToHitGroupIndex) + D3D12_RAYTRACING_INSTANCE_DESC.InstanceContributionToHitGroupIndex)
//
//	GeometryContributionToHitGroupIndex is a system generated index of geometry in bottom-level acceleration structure
//(0,1,2,3..)
//

// This blog post by Will is very useful for calculating sbt in various graphics APIs
// https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways

// RaytracingShaderRecord = {{Shader Identifier}, {RootArguments}}
template<typename T>
struct RaytracingShaderRecord
{
	static_assert(std::is_trivially_copyable_v<T>, "typename T must be trivially copyable");

	RaytracingShaderRecord() noexcept = default;
	RaytracingShaderRecord(const ShaderIdentifier& ShaderIdentifier, const T& RootArguments) noexcept
		: ShaderIdentifier(ShaderIdentifier)
		, RootArguments(RootArguments)
	{
	}

	ShaderIdentifier ShaderIdentifier;
	T				 RootArguments;
};

template<>
struct RaytracingShaderRecord<void>
{
	RaytracingShaderRecord() noexcept = default;
	RaytracingShaderRecord(const ShaderIdentifier& ShaderIdentifier) noexcept
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
class RaytracingShaderTable : public IRaytracingShaderTable
{
public:
	using Record = RaytracingShaderRecord<T>;
	static constexpr UINT64 StrideInBytes =
		AlignUp<UINT64>(sizeof(Record), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	RaytracingShaderTable(size_t NumShaderRecords) { ShaderRecords.resize(NumShaderRecords); }

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
class RaytracingShaderBindingTable
{
public:
	template<typename T>
	RaytracingShaderTable<T>* AddRayGenerationShaderTable(UINT NumRayGenerationShaders)
	{
		assert(RayGenerationShaderTable == nullptr);
		RaytracingShaderTable<T>* Table = new RaytracingShaderTable<T>(NumRayGenerationShaders);
		RayGenerationShaderTable		= std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	template<typename T>
	RaytracingShaderTable<T>* AddMissShaderTable(UINT NumMissShaders)
	{
		assert(MissShaderTable == nullptr);
		RaytracingShaderTable<T>* Table = new RaytracingShaderTable<T>(NumMissShaders);
		MissShaderTable					= std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	template<typename T>
	RaytracingShaderTable<T>* AddHitGroupShaderTable(UINT NumHitGroups)
	{
		assert(HitGroupShaderTable == nullptr);
		RaytracingShaderTable<T>* Table = new RaytracingShaderTable<T>(NumHitGroups);
		HitGroupShaderTable				= std::unique_ptr<IRaytracingShaderTable>(Table);
		return Table;
	}

	// Call this after shader tables are added
	void Generate(Device* Device);

	// Call this after shader records for the tables have been filled out
	void Write();

	// Call this to upload the records to GPU table
	void CopyToGPU(CommandContext& Context) const;

	D3D12_DISPATCH_RAYS_DESC GetDispatchRaysDesc(UINT RayGenerationShaderIndex, UINT BaseMissShaderIndex) const;

private:
	std::unique_ptr<IRaytracingShaderTable> RayGenerationShaderTable;
	std::unique_ptr<IRaytracingShaderTable> MissShaderTable;
	std::unique_ptr<IRaytracingShaderTable> HitGroupShaderTable;

	UINT64 RayGenerationShaderTableOffset = 0;
	UINT64 MissShaderTableOffset		  = 0;
	UINT64 HitGroupShaderTableOffset	  = 0;

	UINT64 SizeInBytes = 0;

	Buffer					SBTBuffer;
	std::unique_ptr<BYTE[]> CPUData;
};
