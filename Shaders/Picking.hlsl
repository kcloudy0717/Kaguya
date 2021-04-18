#include <HLSLCommon.hlsli>

struct SystemConstants
{
	Camera Camera;
	
	// x, y = Resolution
	// z, w = 1 / Resolution
	float4 Resolution;

	float2 MousePosition;

	uint TotalFrameCount;
};

ConstantBuffer<SystemConstants> 	g_SystemConstants 	: register(b0, space0);

RaytracingAccelerationStructure		g_Scene				: register(t0, space0);

RWStructuredBuffer<int>             g_PickingResult     : register(u0, space0);

struct RayPayload
{
    int InstanceID;
};

enum RayType
{
	RayTypePrimary,
    NumRayTypes
};

[shader("raygeneration")]
void RayGeneration()
{
	float2 uv = (g_SystemConstants.MousePosition + float2(0.5f, 0.5f)) / g_SystemConstants.Resolution.xy;

    RayDesc Ray = 
    { 
        g_SystemConstants.Camera.Position.xyz, 
        g_SystemConstants.Camera.NearZ, 
        GenerateWorldCameraRayDirection(uv, g_SystemConstants.Camera), 
        g_SystemConstants.Camera.FarZ 
    };

    RayPayload RayPayload = { -1 };

    const uint RayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
	const uint InstanceInclusionMask = 0xffffffff;
	const uint RayContributionToHitGroupIndex = RayTypePrimary;
	const uint MultiplierForGeometryContributionToHitGroupIndex = NumRayTypes;
	const uint MissShaderIndex = RayTypePrimary;
	TraceRay(g_Scene,
			RayFlags,
			InstanceInclusionMask, 
			RayContributionToHitGroupIndex, 
			MultiplierForGeometryContributionToHitGroupIndex, 
			MissShaderIndex, 
			Ray, 
			RayPayload);

    g_PickingResult[0] = RayPayload.InstanceID;
}

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{

}

[shader("closesthit")]
void ClosestHit(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attrib)
{
    rayPayload.InstanceID = InstanceID();
}