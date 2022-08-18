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
		assert(Vertices.size() <= size_t(std::numeric_limits<u32>::max()));
		assert(Indices.size() <= size_t(std::numeric_limits<u32>::max()));
		assert(Meshlets.size() <= size_t(std::numeric_limits<u32>::max()));
		assert(UniqueVertexIndices.size() <= size_t(std::numeric_limits<u32>::max()));
		assert(PrimitiveIndices.size() <= size_t(std::numeric_limits<u32>::max()));

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

	void Mesh::SetVertices(std::vector<Vertex>&& Vertices)
	{
		this->Vertices = std::move(Vertices);
		NumVertices	   = static_cast<u32>(this->Vertices.size());
	}

	void Mesh::SetIndices(std::vector<u32>&& Indices)
	{
		this->Indices = std::move(Indices);
		NumIndices	  = static_cast<u32>(this->Indices.size());
	}

	void Mesh::SetMeshlets(std::vector<DirectX::Meshlet>&& Meshlets)
	{
		this->Meshlets = std::move(Meshlets);
		NumMeshlets	   = static_cast<u32>(this->Meshlets.size());
	}

	void Mesh::SetUniqueVertexIndices(std::vector<u8>&& UniqueVertexIndices)
	{
		this->UniqueVertexIndices = std::move(UniqueVertexIndices);
		NumVertexIndices		  = static_cast<u32>(this->UniqueVertexIndices.size());
	}

	void Mesh::SetPrimitiveIndices(std::vector<DirectX::MeshletTriangle>&& PrimitiveIndices)
	{
		this->PrimitiveIndices = std::move(PrimitiveIndices);
		NumPrimitives		   = static_cast<u32>(this->PrimitiveIndices.size());
	}

} // namespace Asset
