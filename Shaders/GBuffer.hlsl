#include "HLSLCommon.hlsli"

cbuffer RootConstants : register(b0, space0)
{
	uint MeshIndex;
};

struct GlobalConstants
{
	Camera Camera;

	uint NumLights;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b1, space0);

StructuredBuffer<Material> g_Materials : register(t0, space0);
StructuredBuffer<Light>	   g_Lights : register(t1, space0);
StructuredBuffer<Mesh>	   g_Meshes : register(t2, space0);

#include "DescriptorTable.hlsli"

struct MRT
{
	float4 Albedo : SV_Target0;
	float4 Normal : SV_Target1;
	float2 Motion : SV_Target2;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float4 CurrPosition : CURR_POSITION;
	float4 PrevPosition : PREV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 N : NORMAL;
};

VSOutput VSMain(float3 Position : POSITION, float2 TextureCoord : TEXCOORD, float3 Normal : NORMAL)
{
	VSOutput output;

	Mesh mesh = g_Meshes[MeshIndex];

	output.Position = mul(float4(Position, 1.0f), mesh.Transform);
	output.Position = mul(output.Position, g_GlobalConstants.Camera.ViewProjection);

	output.CurrPosition = mul(float4(Position, 1.0f), mesh.Transform);
	output.CurrPosition = mul(output.CurrPosition, g_GlobalConstants.Camera.ViewProjection);
	output.PrevPosition = mul(float4(Position, 1.0f), mesh.PreviousTransform);
	output.PrevPosition = mul(output.PrevPosition, g_GlobalConstants.Camera.PrevViewProjection);
	output.TexCoord		= TextureCoord;
	output.N			= normalize(mul(Normal, (float3x3)mesh.Transform));

	// float3 t, b;
	// CoordinateSystem(output.N, t, b);

	return output;
}

MRT PSMain(VSOutput input)
{
	Mesh	 mesh	  = g_Meshes[MeshIndex];
	Material material = g_Materials[mesh.MaterialIndex];
	if (material.Albedo != -1)
	{
		// Texture2D texture = ResourceDescriptorHeap[material.Albedo];
		Texture2D texture  = g_Texture2DTable[material.Albedo];
		material.baseColor = texture.Sample(g_SamplerAnisotropicWrap, input.TexCoord).rgb;
	}

	float3 currentPosNDC  = input.CurrPosition.xyz / input.CurrPosition.w;
	float3 previousPosNDC = input.PrevPosition.xyz / input.PrevPosition.w;
	float2 velocity		  = currentPosNDC.xy - previousPosNDC.xy;

	MRT mrt;
	mrt.Albedo = float4(material.baseColor, 1.0f);
	mrt.Normal = float4(input.N, 1.0f);
	mrt.Motion = velocity;
	return mrt;
}
