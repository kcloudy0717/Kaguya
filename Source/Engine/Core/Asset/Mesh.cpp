#include "Mesh.h"

namespace Asset
{
	void Mesh::ComputeBoundingBox()
	{
		DirectX::BoundingBox Box;
		DirectX::BoundingBox::CreateFromPoints(Box, Vertices.size(), &Vertices[0].Position, sizeof(Vertex));
		BoundingBox.Center	= Math::Vec3f(Box.Center.x, Box.Center.y, Box.Center.z);
		BoundingBox.Extents = Math::Vec3f(Box.Extents.x, Box.Extents.y, Box.Extents.z);
	}

	void Mesh::UpdateInfo()
	{
		NumVertices		 = static_cast<u32>(Vertices.size());
		NumIndices		 = static_cast<u32>(Indices.size());
		NumMeshlets		 = static_cast<u32>(Meshlets.size());
		NumVertexIndices = static_cast<u32>(UniqueVertexIndices.size());
		NumPrimitives	 = static_cast<u32>(PrimitiveIndices.size());
	}

	void Mesh::Release()
	{
		decltype(Vertices)().swap(Vertices);
		decltype(Indices)().swap(Indices);
		decltype(Meshlets)().swap(Meshlets);
		decltype(UniqueVertexIndices)().swap(UniqueVertexIndices);
		decltype(PrimitiveIndices)().swap(PrimitiveIndices);
	}

	void Mesh::ResetRaytracingInfo()
	{
		AccelerationStructure = {};
		BlasIndex			  = UINT64_MAX;
		BlasValid			  = false;
		BlasCompacted		  = false;
	}
} // namespace Asset
