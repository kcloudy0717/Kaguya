#include "Vertex.hlsli"
#include "Random.hlsli"
#include "Shader.hlsli"
#include "SharedTypes.hlsli"

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

	Vertex vertex = BarycentricInterpolation(
		vtx0,
		vtx1,
		vtx2,
		float3(
			1.f - Attributes.Barycentrics.x - Attributes.Barycentrics.y,
			Attributes.Barycentrics.x,
			Attributes.Barycentrics.y));
	vertex.Normal = normalize(mul(vertex.Normal, transpose((float3x3)Attributes.ObjectToWorld3x4)));

	VertexAttributes vertexAttributes;
	vertexAttributes.p	= Attributes.WorldRayOrigin + (Attributes.WorldRayDirection * Attributes.RayTCurrent);
	vertexAttributes.Ng = n;
	vertexAttributes.Ns = vertex.Normal;
	vertexAttributes.uv = vertex.TextureCoord;

	return vertexAttributes;
}

#define INVALID_ID (0xDEADBEEF)

[numthreads(16, 16, 1)] void CSMain(CSParams Params)
{
	uint2	launchIndex		 = Params.DispatchThreadID.xy;
	uint2	launchDimensions = g_GlobalConstants.Dimensions;
	Sampler pcgSampler		 = InitSampler(launchIndex, launchDimensions, g_GlobalConstants.TotalFrameCount);

	// Instantiate ray query object.
	// Template parameter allows driver to generate a specialized
	// implemenation.  No specialization in this example.
	RayQuery<RAY_FLAG_NONE> q;

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

	float3 L	= float3(0.0f, 0.0f, 0.0f);
	float3 beta = float3(1.0f, 1.0f, 1.0f);

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
	switch (q.CommittedStatus())
	{
	case COMMITTED_TRIANGLE_HIT:
	{
		RaytracingAttributes ra;
		ra.Barycentrics		 = q.CandidateTriangleBarycentrics();
		ra.PrimitiveIndex	 = q.CandidatePrimitiveIndex();
		ra.ObjectToWorld3x4	 = q.CandidateObjectToWorld3x4();
		ra.WorldRayOrigin	 = q.WorldRayOrigin();
		ra.WorldRayDirection = q.WorldRayDirection();
		ra.RayTCurrent		 = q.CommittedRayT();

		Mesh mesh = g_Meshes[q.CandidateInstanceID()];

		VertexAttributes va = GetVertexAttributes(mesh, ra);
		L					= va.Ng;
	}
	break;

	case COMMITTED_NOTHING:
	{
		float t = 0.5f * (q.WorldRayDirection().y + 1.0f);
		L += beta * lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t) * g_GlobalConstants.SkyIntensity;
	}
	break;
	}

	RWTexture2D<float4> RenderTarget = g_RWTexture2DTable[g_GlobalConstants.RenderTarget];
	// Progressive accumulation
	if (g_GlobalConstants.NumAccumulatedSamples > 0)
	{
		L = lerp(RenderTarget[launchIndex].rgb, L, 1.0f / float(g_GlobalConstants.NumAccumulatedSamples));
	}

	RenderTarget[launchIndex] = float4(L, 1);
}
