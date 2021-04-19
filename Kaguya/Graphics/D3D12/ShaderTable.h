#pragma once
#include <vector>
#include <Core/Math.h>

/*
*	========== Miss shader table indexing ==========
*	Simple pointer arithmetic
*	MissShaderRecordAddress = D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StartAddress + D3D12_DISPATCH_RAYS_DESC.MissShaderTable.StrideInBytes * MissShaderIndex
*/

/*
*	========== Hit group table indexing ==========
*	HitGroupRecordAddress = D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StartAddress + D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StrideInBytes * HitGroupEntryIndex
*
*	where HitGroupEntryIndex =
*	(RayContributionToHitGroupIndex + (MultiplierForGeometryContributionToHitGroupIndex * GeometryContributionToHitGroupIndex) + D3D12_RAYTRACING_INSTANCE_DESC.InstanceContributionToHitGroupIndex)
*
*	GeometryContributionToHitGroupIndex is a system generated index of geometry in bottom-level acceleration structure (0,1,2,3..)
*/

// ShaderRecord = {{Shader Identifier}, {RootArguments}}
template<typename T>
struct ShaderRecord
{
	ShaderRecord() = default;
	ShaderRecord(ShaderIdentifier ShaderIdentifier, T RootArguments)
		: ShaderIdentifier(ShaderIdentifier)
		, RootArguments(RootArguments)
	{

	}

	ShaderIdentifier ShaderIdentifier;
	T RootArguments;
};

template<>
struct ShaderRecord<void>
{
	ShaderRecord() = default;
	ShaderRecord(ShaderIdentifier ShaderIdentifier)
		: ShaderIdentifier(ShaderIdentifier)
	{

	}

	ShaderIdentifier ShaderIdentifier;
};

template<typename T>
class ShaderTable
{
public:
	using Record = ShaderRecord<T>;

	ShaderTable()
		: m_StrideInBytes(AlignUp<UINT64>(sizeof(Record), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT))
		, m_Resource(nullptr)
	{

	}

	operator D3D12_GPU_VIRTUAL_ADDRESS_RANGE() const
	{
		return
		{
			.StartAddress = m_Resource->GetGPUVirtualAddress(),
			.SizeInBytes = GetSizeInBytes()
		};
	}

	operator D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE() const
	{
		return
		{
			.StartAddress = m_Resource->GetGPUVirtualAddress(),
			.SizeInBytes = GetSizeInBytes(),
			.StrideInBytes = GetStrideInBytes()
		};
	}

	auto Resource() const
	{
		return m_Resource;
	}

	void clear()
	{
		m_ShaderRecords.clear();
	}

	void reserve(size_t NumShaderRecords)
	{
		m_ShaderRecords.reserve(NumShaderRecords);
	}

	void resize(size_t NumShaderRecords)
	{
		m_ShaderRecords.resize(NumShaderRecords);
	}

	Record& operator[](size_t Index)
	{
		return m_ShaderRecords[Index];
	}

	const Record& operator[](size_t Index) const
	{
		return m_ShaderRecords[Index];
	}

	void AssociateResource(ID3D12Resource* pResource)
	{
		m_Resource = pResource;
	}

	void AddShaderRecord(const Record& Record)
	{
		m_ShaderRecords.push_back(Record);
	}

	auto GetSizeInBytes() const
	{
		UINT64 SizeInBytes = static_cast<UINT64>(m_ShaderRecords.size()) * m_StrideInBytes;
		SizeInBytes = AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		return SizeInBytes;
	}

	auto GetStrideInBytes() const
	{
		return m_StrideInBytes;
	}

	void Generate(BYTE* pHostMemory)
	{
		if (pHostMemory)
		{
			for (const auto& Record : m_ShaderRecords)
			{
				// Copy record data
				memcpy(pHostMemory, &Record, sizeof(Record));

				pHostMemory += m_StrideInBytes;
			}
		}
	}
private:
	std::vector<Record> m_ShaderRecords;
	UINT64 m_StrideInBytes;
	ID3D12Resource* m_Resource;
};