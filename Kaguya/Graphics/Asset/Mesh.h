#pragma once
#include <memory>
#include <functional>

#include "../RenderDevice.h"
#include "../Vertex.h"

namespace Asset
{
	struct MeshMetadata
	{
		std::filesystem::path Path;
		bool KeepGeometryInRAM;
	};

	struct Submesh
	{
		uint32_t IndexCount;
		uint32_t StartIndexLocation;
		uint32_t VertexCount;
		uint32_t BaseVertexLocation;
	};

	struct Mesh
	{
		MeshMetadata Metadata;

		std::string Name;

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		std::vector<Submesh> Submeshes;

		std::shared_ptr<Resource> VertexResource;
		std::shared_ptr<Resource> IndexResource;
		std::shared_ptr<Resource> AccelerationStructure;
		BottomLevelAccelerationStructure BLAS;
	};
}