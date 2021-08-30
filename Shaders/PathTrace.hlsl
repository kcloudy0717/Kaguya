#include "Global.hlsli"

/*
 * TODO:
 * 1. Add HDRI rendering, and potentially modify the environment color in constant buffer instead or just add a
 * environment cubemap
 * 2. Add PMEMS, and geometry area lights
 * 3. Add volumetric scattering
 */

// Samples per pixel
#define SPP 4

// HitGroup Local Root Signature
// ====================
cbuffer RootConstants : register(b0, space1)
{
	uint MaterialIndex;
};

StructuredBuffer<Vertex> VertexBuffer : register(t0, space1);
StructuredBuffer<uint>	 IndexBuffer : register(t1, space1);

SurfaceInteraction GetSurfaceInteraction(in BuiltInTriangleIntersectionAttributes Attributes)
{
	// Fetch indices
	unsigned int idx0 = IndexBuffer[PrimitiveIndex() * 3 + 0];
	unsigned int idx1 = IndexBuffer[PrimitiveIndex() * 3 + 1];
	unsigned int idx2 = IndexBuffer[PrimitiveIndex() * 3 + 2];

	// Fetch vertices
	Vertex vtx0 = VertexBuffer[idx0];
	Vertex vtx1 = VertexBuffer[idx1];
	Vertex vtx2 = VertexBuffer[idx2];

	float3 p0 = vtx0.Position, p1 = vtx1.Position, p2 = vtx2.Position;
	// Compute 2 edges of the triangle
	float3 e0 = p1 - p0;
	float3 e1 = p2 - p0;
	float3 n  = normalize(cross(e0, e1));
	n		  = normalize(mul(n, transpose((float3x3)ObjectToWorld3x4())));

	float3 barycentrics = float3(
		1.f - Attributes.barycentrics.x - Attributes.barycentrics.y,
		Attributes.barycentrics.x,
		Attributes.barycentrics.y);
	Vertex vertex = BarycentricInterpolation(vtx0, vtx1, vtx2, barycentrics);
	vertex.Normal = normalize(mul(vertex.Normal, transpose((float3x3)ObjectToWorld3x4())));

	SurfaceInteraction si;
	si.p  = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
	si.wo = -WorldRayDirection();
	si.n  = n;
	si.uv = vertex.TextureCoord;

	// Compute geometry basis and shading basis
	si.GeometryFrame = InitFrameFromZ(n);
	si.ShadingFrame	 = InitFrameFromZ(vertex.Normal);

	// Update BSDF's internal data
	Material material = g_Materials[MaterialIndex];

	if (material.Albedo != -1)
	{
		// Texture2D Texture = ResourceDescriptorHeap[material.Albedo];
		Texture2D Texture  = g_Texture2DTable[material.Albedo];
		material.baseColor = Texture.SampleLevel(g_SamplerAnisotropicWrap, si.uv, 0.0f).rgb;
	}

	si.BSDF = InitBSDF(si.GeometryFrame.z, si.ShadingFrame, material);

	return si;
}

struct RayPayload
{
	RayDesc SpawnRay(float3 d)
	{
		RayDesc ray = { p, 0.001f, normalize(d), 100000.0f };
		return ray;
	}

	float3 L;
	float3 beta;
	float3 p;
	float3 wi;
	uint   Seed;
	uint   Depth;
};

struct ShadowRayPayload
{
	float Visibility; // 1.0f for fully lit, 0.0f for fully shadowed
};

enum RayType
{
	RayTypePrimary,
	NumRayTypes
};

float TraceShadowRay(RayDesc DXRRay)
{
	ShadowRayPayload RayPayload = { 0.0f };

	// Trace the ray
	const uint RayFlags =
		RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;
	const uint InstanceInclusionMask							= 0xffffffff;
	const uint RayContributionToHitGroupIndex					= RayTypePrimary;
	const uint MultiplierForGeometryContributionToHitGroupIndex = NumRayTypes;
	const uint MissShaderIndex									= 1;
	TraceRay(
		g_Scene,
		RayFlags,
		InstanceInclusionMask,
		RayContributionToHitGroupIndex,
		MultiplierForGeometryContributionToHitGroupIndex,
		MissShaderIndex,
		DXRRay,
		RayPayload);

	return RayPayload.Visibility;
}

float3 EstimateDirect(SurfaceInteraction si, Light light, float2 XiLight)
{
	float3 Ld = float3(0.0f, 0.0f, 0.0f);

	// Sample light source with multiple importance sampling
	float3			 wi;
	float			 lightPdf = 0.0f, scatteringPdf = 0.0f;
	VisibilityTester visibilityTester;

	float3 Li = SampleLi(light, si, XiLight, wi, lightPdf, visibilityTester);
	if (lightPdf > 0.0f && any(Li))
	{
		// Compute BSDF's value for light sample
		// Evaluate BSDF for light sampling strategy
		float3 f	  = si.BSDF.f(si.wo, wi) * abs(dot(wi, si.ShadingFrame.z));
		scatteringPdf = si.BSDF.Pdf(si.wo, wi);

		float visibility = TraceShadowRay(visibilityTester.I0.SpawnRayTo(visibilityTester.I1));

		// Add light's contribution to reflected radiance
		if (light.Type == LightType_Point)
		{
			Ld += f * Li * visibility / lightPdf;
		}

		if (light.Type == LightType_Quad)
		{
			float weight = PowerHeuristic(1, lightPdf, 1, scatteringPdf);
			Ld += f * Li * weight * visibility / lightPdf;
		}
	}

	// No BSDF MIS yet

	return Ld;
}

float3 UniformSampleOneLight(SurfaceInteraction si, inout uint seed)
{
	if (g_SystemConstants.NumLights == 0)
	{
		return float3(0.0f, 0.0f, 0.0f);
	}

	int numLights = g_SystemConstants.NumLights;

	int	  lightIndex = min(RandomFloat01(seed) * numLights, numLights - 1);
	float lightPdf	 = 1.0f / float(numLights);

	Light light = g_Lights[lightIndex];

	float2 XiLight = float2(RandomFloat01(seed), RandomFloat01(seed));

	return EstimateDirect(si, light, XiLight) / lightPdf;
}

void TerminateRay(inout RayPayload rayPayload)
{
    rayPayload.Depth = g_SystemConstants.MaxDepth;
}

float3 Li(RayDesc DXRRay, inout uint Seed)
{
	RayPayload RayPayload = {
		float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f), DXRRay.Origin, DXRRay.Direction, Seed, 0
	};

    while (RayPayload.Depth < g_SystemConstants.MaxDepth)
	{
		// Trace the ray
		const uint RayFlags											= RAY_FLAG_NONE;
		const uint InstanceInclusionMask							= 0xffffffff;
		const uint RayContributionToHitGroupIndex					= RayTypePrimary;
		const uint MultiplierForGeometryContributionToHitGroupIndex = NumRayTypes;
		const uint MissShaderIndex									= 0;
		TraceRay(
			g_Scene,
			RayFlags,
			InstanceInclusionMask,
			RayContributionToHitGroupIndex,
			MultiplierForGeometryContributionToHitGroupIndex,
			MissShaderIndex,
			DXRRay,
			RayPayload);

		// Spawn a new ray based on the sampled direction from the BSDF
		DXRRay = RayPayload.SpawnRay(RayPayload.wi);

		RayPayload.Depth++;
	}

	return RayPayload.L;
}

[shader("raygeneration")]
void RayGeneration()
{
	const uint2 launchIndex		 = DispatchRaysIndex().xy;
	const uint2 launchDimensions = DispatchRaysDimensions().xy;
	uint		seed			 = uint(
					   launchIndex.x * uint(1973) + launchIndex.y * uint(9277) +
					   uint(g_SystemConstants.TotalFrameCount) * uint(26699)) |
				uint(1);

	float3 L = float3(0.0f, 0.0f, 0.0f);
	for (int s = 0; s < SPP; ++s)
	{
		// TODO: Replace sampler with sobol
		// Calculate subpixel camera jitter for anti aliasing
		const float2 jitter = float2(RandomFloat01(seed), RandomFloat01(seed)) - 0.5f;
		const float2 pixel	= (float2(launchIndex) + jitter) / float2(launchDimensions);

		const float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

		// Initialize ray
		RayDesc ray = g_SystemConstants.Camera.GenerateCameraRay(ndc, seed);

		L += Li(ray, seed);
	}
	L /= float(SPP);

	// Replace NaN components with zero. See explanation in Ray Tracing: The Rest of Your Life.
	L.r = isnan(L.r) ? 0.0f : L.r;
	L.g = isnan(L.g) ? 0.0f : L.g;
	L.b = isnan(L.b) ? 0.0f : L.b;

	// RWTexture2D<float4> RenderTarget = ResourceDescriptorHeap[g_SystemConstants.RenderTarget];
    RWTexture2D<float4> RenderTarget = g_RWTexture2DTable[g_SystemConstants.RenderTarget];

	// Progressive accumulation
    if (g_SystemConstants.NumAccumulatedSamples > 0)
	{
        L = lerp(RenderTarget[launchIndex].rgb, L, 1.0f / float(g_SystemConstants.NumAccumulatedSamples));
    }

	RenderTarget[launchIndex] = float4(L, 1);
}

[shader("miss")]
void Miss(inout RayPayload rayPayload : SV_RayPayload)
{
    float t = 0.5f * (WorldRayDirection().y + 1.0f);
    rayPayload.L += rayPayload.beta * lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t) * g_SystemConstants.SkyIntensity;
	TerminateRay(rayPayload);
}

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload rayPayload : SV_RayPayload)
{
	// If we miss all geometry, then the light is visible
	rayPayload.Visibility = 1.0f;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload rayPayload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes Attributes)
{
	SurfaceInteraction si = GetSurfaceInteraction(Attributes);

	// Sample illumination from lights to find path contribution.
	// (But skip this for perfectly specular BSDFs.)
	if (si.BSDF.IsNonSpecular())
	{
		rayPayload.L += rayPayload.beta * UniformSampleOneLight(si, rayPayload.Seed);
	}

	// Sample BSDF to get new path direction
	float3	   wo		  = -WorldRayDirection();
	BSDFSample bsdfSample = (BSDFSample)0;
	bool	   success =
		si.BSDF.Samplef(wo, float2(RandomFloat01(rayPayload.Seed), RandomFloat01(rayPayload.Seed)), bsdfSample);
	if (!success)
	{
		// Used to debug
		// rayPayload.L = bsdfSample.f;
		TerminateRay(rayPayload);
		return;
	}

	rayPayload.beta *= bsdfSample.f * abs(dot(bsdfSample.wi, si.ShadingFrame.z)) / bsdfSample.pdf;

	// Spawn new ray
	rayPayload.p  = si.p;
	rayPayload.wi = bsdfSample.wi;

	const float rrThreshold			= 1.0f;
	float3		rr					= rayPayload.beta;
	float		rrMaxComponentValue = max(rr.x, max(rr.y, rr.z));
	if (rrMaxComponentValue < rrThreshold && rayPayload.Depth > 1)
	{
		float q = max(0.0f, 1.0f - rrMaxComponentValue);
		if (RandomFloat01(rayPayload.Seed) < q)
		{
			TerminateRay(rayPayload);
		}
		rayPayload.beta /= 1.0f - q;
	}
}
