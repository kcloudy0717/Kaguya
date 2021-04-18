typedef uint64_t D3D12_GPU_VIRTUAL_ADDRESS;

struct RTObject
{
	float3x4 Transform;
	uint InstanceIDAndMask;
	uint InstanceContributionToHitGroupIndexAndFlags;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;
};

struct D3D12_RAYTRACING_INSTANCE_DESC
{
	float3x4 Transform;
	uint InstanceIDAndMask;
	uint InstanceContributionToHitGroupIndexAndFlags;
    D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;
};

#include "SharedTypes.hlsli"

cbuffer RootConstants
{
	int NumInstances;
};

StructuredBuffer<RTObject> RTObjects : register(t0, space0);
RWStructuredBuffer<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs : register(u0, space0);

[numthreads(64, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	uint index = DTid.x;
	if (index < NumInstances)
	{
		RTObject rtObject = RTObjects[index];
		
		D3D12_RAYTRACING_INSTANCE_DESC desc;
		desc.Transform = rtObject.Transform;
		desc.InstanceIDAndMask = rtObject.InstanceIDAndMask;
		desc.InstanceContributionToHitGroupIndexAndFlags = rtObject.InstanceContributionToHitGroupIndexAndFlags;
		desc.AccelerationStructure = rtObject.AccelerationStructure;

		InstanceDescs[index] = desc;
	}
}