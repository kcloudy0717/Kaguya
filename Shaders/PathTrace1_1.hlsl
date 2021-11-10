#include "Random.hlsli"
#include "Shader.hlsli"
#include "SharedTypes.hlsli"

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
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b0, space0);

RaytracingAccelerationStructure g_Scene : register(t0, space0);
StructuredBuffer<Material>		g_Materials : register(t1, space0);
StructuredBuffer<Light>			g_Lights : register(t2, space0);

#include "DescriptorTable.hlsli"

[numthreads(16, 16, 1)] void CSMain(CSParams Params)
{
	RWTexture2D<float4> RenderTarget = g_RWTexture2DTable[g_GlobalConstants.RenderTarget];

	uint2	launchIndex		 = Params.DispatchThreadID.xy;
	uint2	launchDimensions = g_GlobalConstants.Dimensions;
	Sampler pcgSampler		 = InitSampler(launchIndex, launchDimensions, g_GlobalConstants.TotalFrameCount);

	// Instantiate ray query object.
	// Template parameter allows driver to generate a specialized
	// implemenation.  No specialization in this example.
	RayQuery<RAY_FLAG_NONE> q;

	// Calculate subpixel camera jitter for anti aliasing
	float2 jitter = pcgSampler.Get2D() - 0.5f;
	float2 pixel  = (float2(launchIndex) + jitter) / float2(launchDimensions);

	float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

	// Initialize ray
	RayDesc ray = g_GlobalConstants.Camera.GenerateCameraRay(ndc);

	// Set up a trace
	q.TraceRayInline(g_Scene, RAY_FLAG_NONE, 0xff, ray);

	// Proceed() below is where behind-the-scenes traversal happens,
	// including the heaviest of any driver inlined code.
	// In this simplest of scenarios, Proceed() only needs
	// to be called once rather than a loop:
	// Based on the template specialization above,
	// traversal completion is guaranteed.
	q.Proceed();

	// Examine and act on the result of the traversal.
	// Was a hit committed?
	if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		RenderTarget[launchIndex] = float4(q.CandidateTriangleBarycentrics(), 0, 1);
	}
	else
	{
		RenderTarget[launchIndex] = float4(0, 0, 0, 1);
	}
}
