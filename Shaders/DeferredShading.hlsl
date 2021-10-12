#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "HLSLCommon.hlsli"
#include "DescriptorTable.hlsli"

struct GlobalConstants
{
	Camera Camera;

	uint NumMeshes;
	uint NumLights;

	uint AlbedoId;
	uint NormalId;
	uint MotionId;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b1, space0);

StructuredBuffer<Material> g_Materials : register(t0, space0);
StructuredBuffer<Light>	   g_Lights : register(t1, space0);
StructuredBuffer<Mesh>	   g_Meshes : register(t2, space0);

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D Albedo = g_Texture2DTable[g_GlobalConstants.AlbedoId];
	Texture2D Normal = g_Texture2DTable[g_GlobalConstants.NormalId];
	Texture2D Motion = g_Texture2DTable[g_GlobalConstants.MotionId];
}
