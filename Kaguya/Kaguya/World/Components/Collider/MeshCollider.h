#pragma once

struct MeshCollider : Component
{
	std::span<Vertex>		Vertices;
	std::span<unsigned int> Indices;
};
