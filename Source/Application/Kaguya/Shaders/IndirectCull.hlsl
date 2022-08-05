#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "Math.hlsli"
#include "SharedTypes.hlsli"

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

		bool visible = FrustumContainsBoundingBox(g_ConstantBufferParams.Camera.Frustum, aabb) != CONTAINMENT_DISJOINT;
		if (visible)
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
