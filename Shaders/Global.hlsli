#ifndef GLOBAL_HLSLI
#define GLOBAL_HLSLI
// This file defines global root signature for raytracing shaders

#include <HLSLCommon.hlsli>

struct GlobalConstants
{
	Camera Camera;

	// x, y = Resolution
	// z, w = 1 / Resolution
	float4 Resolution;

	float2 MousePosition;

	uint NumLights;
	uint TotalFrameCount;

	uint MaxDepth;
	uint NumAccumulatedSamples;

	uint RenderTarget;

	float SkyIntensity;
};

ConstantBuffer<GlobalConstants> g_SystemConstants : register(b0, space0);

RaytracingAccelerationStructure g_Scene : register(t0, space0);
StructuredBuffer<Material>		g_Materials : register(t1, space0);
StructuredBuffer<Light>			g_Lights : register(t2, space0);

SamplerState g_SamplerPointWrap : register(s0, space0);
SamplerState g_SamplerPointClamp : register(s1, space0);
SamplerState g_SamplerLinearWrap : register(s2, space0);
SamplerState g_SamplerLinearClamp : register(s3, space0);
SamplerState g_SamplerAnisotropicWrap : register(s4, space0);
SamplerState g_SamplerAnisotropicClamp : register(s5, space0);

#include <DescriptorTable.hlsli>

#endif // GLOBAL_HLSLI
