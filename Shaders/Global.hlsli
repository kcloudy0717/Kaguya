#pragma once
// This file defines global root signature for raytracing shaders

#include "HLSLCommon.hlsli"

struct GlobalConstants
{
	Camera Camera;

	// x, y = Resolution
	// z, w = 1 / Resolution
	float4 Resolution;

	uint NumLights;
	uint TotalFrameCount;

	uint MaxDepth;
	uint NumAccumulatedSamples;

	uint RenderTarget;

	float SkyIntensity;

	uint2 Dimensions;
	uint  AntiAliasing;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b0, space0);

RaytracingAccelerationStructure g_Scene : register(t0, space0);
StructuredBuffer<Material>		g_Materials : register(t1, space0);
StructuredBuffer<Light>			g_Lights : register(t2, space0);
