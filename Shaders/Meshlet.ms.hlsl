#include "Shader.hlsli"
#include "Vertex.hlsli"
#include "HLSLCommon.hlsli"
#include "DescriptorTable.hlsli"

struct Meshlet
{
	uint VertexCount;
	uint VertexOffset;
	uint PrimitiveCount;
	uint PrimitiveOffset;
};

cbuffer RootConstants : register(b0, space0)
{
	uint MeshIndex;
};

StructuredBuffer<Vertex>  Vertices : register(t0, space0);
StructuredBuffer<Meshlet> Meshlets : register(t1, space0);
ByteAddressBuffer		  UniqueVertexIndices : register(t2, space0);
StructuredBuffer<uint>	  PrimitiveIndices : register(t3, space0);

struct GlobalConstants
{
	Camera Camera;

	uint NumMeshes;
	uint NumLights;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b0, space1);
StructuredBuffer<Material>		g_Materials : register(t0, space1);
StructuredBuffer<Mesh>			g_Meshes : register(t1, space1);

struct VertexAttributes
{
	float4 Position : SV_POSITION;
	float4 CurrPosition : CURR_POSITION;
	float4 PrevPosition : PREV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 N : NORMAL;
	uint   MeshletIndex : MESHLETINDEX;
};

struct MRT
{
	float4 Albedo : SV_Target0;
	float4 Normal : SV_Target1;
	float2 Motion : SV_Target2;
};

uint3 UnpackPrimitive(uint primitive)
{
	// Unpacks a 10 bits per index triangle from a 32-bit uint.
	return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet meshlet, uint index)
{
	return UnpackPrimitive(PrimitiveIndices[meshlet.PrimitiveOffset + index]);
}

uint GetVertexIndex(Meshlet meshlet, uint localIndex)
{
	localIndex = meshlet.VertexOffset + localIndex;
	return UniqueVertexIndices.Load(localIndex * 4);
}

VertexAttributes GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    Mesh mesh = g_Meshes[MeshIndex];
    Vertex v = Vertices[vertexIndex];

    VertexAttributes output;
    output.Position = mul(float4(v.Position, 1.0f), mesh.Transform);
    output.Position = mul(output.Position, g_GlobalConstants.Camera.ViewProjection);

    output.CurrPosition = mul(float4(v.Position, 1.0f), mesh.Transform);
    output.CurrPosition = mul(output.CurrPosition, g_GlobalConstants.Camera.ViewProjection);
    output.PrevPosition = mul(float4(v.Position, 1.0f), mesh.PreviousTransform);
    output.PrevPosition = mul(output.PrevPosition, g_GlobalConstants.Camera.PrevViewProjection);
    output.TexCoord = v.TextureCoord;
    output.N = normalize(mul(v.Normal, (float3x3) mesh.Transform));
    output.MeshletIndex = meshletIndex;

    return output;
}

#define INDICES	 indices
#define VERTICES vertices

[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void MSMain(
	MSParams					  params,
	out INDICES uint3			  primitives[128],
	out VERTICES VertexAttributes vertices[128])
{
	Meshlet meshlet = Meshlets[params.GroupID.x];
	SetMeshOutputCounts(meshlet.VertexCount, meshlet.PrimitiveCount);

	if (params.GroupThreadID.x < meshlet.PrimitiveCount)
	{
		primitives[params.GroupThreadID.x] = GetPrimitive(meshlet, params.GroupThreadID.x);
	}

	if (params.GroupThreadID.x < meshlet.VertexCount)
	{
		uint vertexIndex = GetVertexIndex(meshlet, params.GroupThreadID.x);
		vertices[params.GroupThreadID.x] = GetVertexAttributes(params.GroupThreadID.x, vertexIndex);
	}
}

MRT PSMain(VertexAttributes input)
{
	Mesh	 mesh	  = g_Meshes[MeshIndex];
	Material material = g_Materials[mesh.MaterialIndex];
	if (material.Albedo != -1)
	{
		// Texture2D texture = ResourceDescriptorHeap[material.Albedo];
		Texture2D texture  = g_Texture2DTable[material.Albedo];
		material.baseColor = texture.Sample(g_SamplerAnisotropicWrap, input.TexCoord).rgb;
	}

	uint meshletIndex  = input.MeshletIndex;
	material.baseColor = float3(float(meshletIndex & 1), float(meshletIndex & 3) / 4, float(meshletIndex & 7) / 8);

	float3 currentPosNDC  = input.CurrPosition.xyz / input.CurrPosition.w;
	float3 previousPosNDC = input.PrevPosition.xyz / input.PrevPosition.w;
	float2 velocity		  = currentPosNDC.xy - previousPosNDC.xy;

	MRT mrt;
	mrt.Albedo = float4(material.baseColor, 1.0f);
	mrt.Normal = float4(input.N, 1.0f);
	mrt.Motion = velocity;
	return mrt;
}
