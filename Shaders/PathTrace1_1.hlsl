#include "Vertex.hlsli"
#include "Random.hlsli"
#include "Shader.hlsli"
#include "SharedTypes.hlsli"
#include "HLSLCommon.hlsli"

// Samples per pixel
#define SPP 4

struct RaySystemValues
{
	float3 WorldRayOrigin;
	float3 WorldRayDirection;
	float  RayTCurrent;
};

struct RaytracingAttributes
{
	float2	 Barycentrics;
	uint	 PrimitiveIndex;
	float3x4 ObjectToWorld3x4;
	float3	 WorldRayOrigin;
	float3	 WorldRayDirection;
	float3	 RayTCurrent;
};

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
	int	  Sky;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b0, space0);

RaytracingAccelerationStructure g_Scene : register(t0, space0);
StructuredBuffer<Material>		g_Materials : register(t1, space0);
StructuredBuffer<Light>			g_Lights : register(t2, space0);
StructuredBuffer<Mesh>			g_Meshes : register(t3, space0);

#include "DescriptorTable.hlsli"

struct VertexAttributes
{
	float3 p;
	float3 Ng;
	float3 Ns;
	float2 uv;
};

VertexAttributes GetVertexAttributes(Mesh Mesh, RaytracingAttributes Attributes)
{
	// Fetch indices
	uint3 indices = g_ByteAddressBufferTable[Mesh.IndexView].Load<uint3>(Attributes.PrimitiveIndex * sizeof(uint3));
	uint  idx0	  = indices[0];
	uint  idx1	  = indices[1];
	uint  idx2	  = indices[2];

	// Fetch vertices
	Vertex vtx0 = g_ByteAddressBufferTable[Mesh.VertexView].Load<Vertex>(idx0 * sizeof(Vertex));
	Vertex vtx1 = g_ByteAddressBufferTable[Mesh.VertexView].Load<Vertex>(idx1 * sizeof(Vertex));
	Vertex vtx2 = g_ByteAddressBufferTable[Mesh.VertexView].Load<Vertex>(idx2 * sizeof(Vertex));

	float3 p0 = vtx0.Position, p1 = vtx1.Position, p2 = vtx2.Position;
	// Compute 2 edges of the triangle
	float3 e0 = p1 - p0;
	float3 e1 = p2 - p0;
	float3 n  = normalize(cross(e0, e1));
	n		  = normalize(mul(n, transpose((float3x3)Attributes.ObjectToWorld3x4)));

	Vertex vertex = BarycentricInterpolation(vtx0, vtx1, vtx2, float3(1.f - Attributes.Barycentrics.x - Attributes.Barycentrics.y, Attributes.Barycentrics.x, Attributes.Barycentrics.y));
	vertex.Normal = normalize(mul(vertex.Normal, transpose((float3x3)Attributes.ObjectToWorld3x4)));

	VertexAttributes vertexAttributes;
	vertexAttributes.p	= Attributes.WorldRayOrigin + (Attributes.WorldRayDirection * Attributes.RayTCurrent);
	vertexAttributes.Ng = n;
	vertexAttributes.Ns = vertex.Normal;
	vertexAttributes.uv = vertex.TextureCoord;

	return vertexAttributes;
}

float TraceShadowRay(RayDesc Ray)
{
	RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER> q;
	q.TraceRayInline(g_Scene, RAY_FLAG_NONE, 0xff, Ray);
	q.Proceed();
	return q.CommittedStatus() == COMMITTED_NOTHING;
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

float3 UniformSampleOneLight(SurfaceInteraction si, inout Sampler Sampler)
{
	if (g_GlobalConstants.NumLights == 0)
	{
		return float3(0.0f, 0.0f, 0.0f);
	}

	int numLights = g_GlobalConstants.NumLights;

	int	  lightIndex = min(Sampler.Get1D() * numLights, numLights - 1);
	float lightPdf	 = 1.0f / float(numLights);

	Light light = g_Lights[lightIndex];

	return EstimateDirect(si, light, Sampler.Get2D()) / lightPdf;
}

float3 Li(RayDesc ray, inout Sampler Sampler)
{
	float3 L	= float3(0.0f, 0.0f, 0.0f);
	float3 beta = float3(1.0f, 1.0f, 1.0f);

	for (int bounce = 0; bounce < g_GlobalConstants.MaxDepth; ++bounce)
	{
		RayQuery<RAY_FLAG_NONE> q;
		q.TraceRayInline(g_Scene, RAY_FLAG_NONE, 0xff, ray);
		q.Proceed();
		COMMITTED_STATUS status = q.CommittedStatus();

		if (status == COMMITTED_NOTHING)
		{
			if (g_GlobalConstants.Sky != -1)
			{
				TextureCube Sky = g_TextureCubeTable[g_GlobalConstants.Sky];
				L += beta * Sky.SampleLevel(g_SamplerLinearWrap, q.WorldRayDirection(), 0.0f).rgb *
					 g_GlobalConstants.SkyIntensity;
			}
			else
			{
				float t = 0.5f * (q.WorldRayDirection().y + 1.0f);
				L += beta * lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t) * g_GlobalConstants.SkyIntensity;
			}
			break;
		}

		RaytracingAttributes ra;
		ra.Barycentrics		 = q.CandidateTriangleBarycentrics();
		ra.PrimitiveIndex	 = q.CandidatePrimitiveIndex();
		ra.ObjectToWorld3x4	 = q.CandidateObjectToWorld3x4();
		ra.WorldRayOrigin	 = q.WorldRayOrigin();
		ra.WorldRayDirection = q.WorldRayDirection();
		ra.RayTCurrent		 = q.CommittedRayT();

		Mesh	 mesh	  = g_Meshes[q.CandidateInstanceID()];
		Material material = g_Materials[mesh.MaterialIndex];

		VertexAttributes va = GetVertexAttributes(mesh, ra);

		float3 Ng = va.Ng;
		float3 Ns = va.Ns;

		float3 wo = -ray.Direction;

		if (material.BSDFType != 2 /* Glass */)
		{
			if (dot(Ng, wo) < 0.0f)
				Ng = -Ng;
			if (dot(Ns, wo) < 0.0f)
				Ns = -Ns;
		}

		SurfaceInteraction si;
		si.p  = va.p;
		si.wo = wo;
		si.n  = Ng;
		si.uv = va.uv;

		// Compute geometry basis and shading basis
		si.GeometryFrame = InitFrameFromZ(Ng);
		si.ShadingFrame	 = InitFrameFromZ(Ns);

		// Update BSDF's internal data

		if (material.Albedo != -1)
		{
			// Texture2D Texture = ResourceDescriptorHeap[material.Albedo];
			Texture2D Texture  = g_Texture2DTable[material.Albedo];
			material.baseColor = Texture.SampleLevel(g_SamplerAnisotropicWrap, si.uv, 0.0f).rgb;
		}

		si.BSDF = InitBSDF(si.GeometryFrame.z, si.ShadingFrame, material);

		// Sample illumination from lights to find path contribution.
		// (But skip this for perfectly specular BSDFs.)
		if (si.BSDF.IsNonSpecular())
		{
			L += beta * UniformSampleOneLight(si, Sampler);
		}

		// Sample BSDF to get new path direction
		BSDFSample bsdfSample = (BSDFSample)0;
		bool	   success	  = si.BSDF.Samplef(si.wo, Sampler.Get2D(), bsdfSample);
		if (!success)
		{
			// Used to debug
			// L = bsdfSample.f;
			break;
		}

		beta *= bsdfSample.f * abs(dot(bsdfSample.wi, si.ShadingFrame.z)) / bsdfSample.pdf;

		// Spawn a new ray based on the sampled direction from the BSDF
		ray = si.SpawnRay(bsdfSample.wi);

		const float rrThreshold			= 1.0f;
		float3		rr					= beta;
		float		rrMaxComponentValue = max(rr.x, max(rr.y, rr.z));
		if (rrMaxComponentValue < rrThreshold && bounce > 1)
		{
			float q = max(0.0f, 1.0f - rrMaxComponentValue);
			if (Sampler.Get1D() < q)
			{
				break;
			}
			beta /= 1.0f - q;
		}
	}

	return L;
}

#define INVALID_ID (0xDEADBEEF)

[numthreads(16, 16, 1)] void CSMain(CSParams Params)
{
	uint2	launchIndex		 = Params.DispatchThreadID.xy;
	uint2	launchDimensions = g_GlobalConstants.Dimensions;
	Sampler pcgSampler		 = InitSampler(launchIndex, launchDimensions, g_GlobalConstants.TotalFrameCount);

	float3 L = float3(0.0f, 0.0f, 0.0f);
	for (int s = 0; s < SPP; ++s)
	{
		// Calculate subpixel camera jitter for anti aliasing
		float2 jitter = pcgSampler.Get2D() - 0.5f;
		float2 pixel;
		if (g_GlobalConstants.AntiAliasing)
		{
			pixel = (float2(launchIndex) + jitter) / float2(launchDimensions);
		}
		else
		{
			pixel = float2(launchIndex) / float2(launchDimensions);
		}

		float2 ndc = float2(2, -2) * pixel + float2(-1, 1);

		// Initialize ray
		RayDesc ray = g_GlobalConstants.Camera.GenerateCameraRay(ndc);

		L += Li(ray, pcgSampler);
	}
	L /= float(SPP);

	// Replace NaN components with zero. See explanation in Ray Tracing: The Rest of Your Life.
	L.r = isnan(L.r) ? 0.0f : L.r;
	L.g = isnan(L.g) ? 0.0f : L.g;
	L.b = isnan(L.b) ? 0.0f : L.b;

	RWTexture2D<float4> RenderTarget = g_RWTexture2DTable[g_GlobalConstants.RenderTarget];
	// Progressive accumulation
	if (g_GlobalConstants.NumAccumulatedSamples > 0)
	{
		L = lerp(RenderTarget[launchIndex].rgb, L, 1.0f / float(g_GlobalConstants.NumAccumulatedSamples));
	}

	RenderTarget[launchIndex] = float4(L, 1);
}
