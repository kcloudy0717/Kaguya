#include "HLSLCommon.hlsli"

struct CSParams
{
	// Indices for which thread group a CS is executing in.
	uint3 GroupID : SV_GroupID;

	// Global thread id w.r.t to the entire dispatch call
	// Ranging from [0, numthreads * ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ))^3
	uint3 DispatchThreadID : SV_DispatchThreadID;

	// Local thread id w.r.t the current thread group
	// Ranging from [0, numthreads)^3
	uint3 GroupThreadID : SV_GroupThreadID;

	// The "flattened" index of a compute shader thread within a thread group, which turns the multi-dimensional
	// SV_GroupThreadID into a 1D value. SV_GroupIndex varies from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1.
	uint GroupIndex : SV_GroupIndex;
};

struct GlobalConstants
{
	Camera Camera;

	uint NumMeshes;
	uint NumLights;
};

ConstantBuffer<GlobalConstants> g_GlobalConstants : register(b0, space0);
StructuredBuffer<Mesh>			g_Meshes : register(t0, space0);
RWStructuredBuffer<uint>		g_VisibilityBuffer : register(u0, space0);

[numthreads(128, 1, 1)] void CSMain(CSParams Params)
{
	// Each thread of the CS operates on one of the indirect commands.
	uint index = (Params.GroupID.x * 128) + Params.GroupIndex;

	// Don't attempt to access commands that don't exist if more threads are allocated
	// than commands.
	if (index < g_GlobalConstants.NumMeshes)
	{
		Mesh mesh = g_Meshes[index];

		BoundingBox aabb;
		mesh.BoundingBox.Transform(mesh.Transform, aabb);

		if (FrustumContainsBoundingBox(g_GlobalConstants.Camera.Frustum, aabb))
		{
			g_VisibilityBuffer[index] = 1;
		}
		else
		{
			g_VisibilityBuffer[index] = 0;
		}
	}
}
