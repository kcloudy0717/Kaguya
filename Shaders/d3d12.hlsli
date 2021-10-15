#pragma once

// Using uint2 version of D3D12_GPU_VIRTUAL_ADDRESS because command signature arguments needs to be 4 byte aligned
// if i use uint64_t, it breaks alignment rules
typedef uint2 D3D12_GPU_VIRTUAL_ADDRESS;
// typedef uint64_t D3D12_GPU_VIRTUAL_ADDRESS;

struct D3D12_DRAW_ARGUMENTS
{
	uint VertexCountPerInstance;
	uint InstanceCount;
	uint StartVertexLocation;
	uint StartInstanceLocation;
};

struct D3D12_DRAW_INDEXED_ARGUMENTS
{
	uint IndexCountPerInstance;
	uint InstanceCount;
	uint StartIndexLocation;
	int	 BaseVertexLocation;
	uint StartInstanceLocation;
};

struct D3D12_DISPATCH_ARGUMENTS
{
	uint ThreadGroupCountX;
	uint ThreadGroupCountY;
	uint ThreadGroupCountZ;
};

struct D3D12_DISPATCH_MESH_ARGUMENTS
{
	uint ThreadGroupCountX;
	uint ThreadGroupCountY;
	uint ThreadGroupCountZ;
};

struct D3D12_VERTEX_BUFFER_VIEW
{
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	uint					  SizeInBytes;
	uint					  StrideInBytes;
};

struct D3D12_INDEX_BUFFER_VIEW
{
	D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
	uint					  SizeInBytes;
	uint					  Format;
};

struct D3D12_RAYTRACING_INSTANCE_DESC
{
	float3x4 Transform;
	// Waiting for dxc bitfield support :D
	// uint InstanceID : 24;
	// uint InstanceMask : 8;
	// uint InstanceContributionToHitGroupIndex : 24;
	// uint Flags : 8;
	uint					  Instance_ID_Mask;
	uint					  Instance_ContributionToHitGroupIndex_Flags;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;

	uint GetID() { return Instance_ID_Mask & 0xFFFFFF00; }
	uint GetMask() { return Instance_ID_Mask & 0x000000FF; }

	uint GetContributionToHitGroupIndex() { return Instance_ContributionToHitGroupIndex_Flags & 0xFFFFFF00; }
	uint GetFlags() { return Instance_ContributionToHitGroupIndex_Flags & 0x000000FF; }
};
