#include "d3d12.hlsli"
#include "Math.hlsli"
#include "SharedTypes.hlsli"

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

struct ConstantBufferParams
{
	Camera Camera;

	uint NumMeshes;
	uint NumLights;
};

struct CommandSignatureParams
{
	uint						 MeshIndex;
	D3D12_VERTEX_BUFFER_VIEW	 VertexBuffer;
	D3D12_INDEX_BUFFER_VIEW		 IndexBuffer;
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
};

ConstantBuffer<ConstantBufferParams>		   g_ConstantBufferParams : register(b0, space0);
StructuredBuffer<Mesh>						   g_Meshes : register(t0, space0);
AppendStructuredBuffer<CommandSignatureParams> g_CommandBuffer : register(u0, space0);

[numthreads(128, 1, 1)] void CSMain(CSParams Params)
{
	// Each thread processes one mesh instance
	// Compute index and ensure is within bounds
	uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
	if (index < g_ConstantBufferParams.NumMeshes)
	{
		Mesh mesh = g_Meshes[index];

		BoundingBox aabb;
		mesh.BoundingBox.Transform(mesh.Transform, aabb);

		if (FrustumContainsBoundingBox(g_ConstantBufferParams.Camera.Frustum, aabb))
		{
			CommandSignatureParams command;
			command.MeshIndex			 = index;
			command.VertexBuffer		 = mesh.VertexBuffer;
			command.IndexBuffer			 = mesh.IndexBuffer;
			command.DrawIndexedArguments = mesh.DrawIndexedArguments;
			g_CommandBuffer.Append(command);
		}
	}
}
