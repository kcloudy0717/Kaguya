#pragma once

class MeshColliderComponent
{
public:
	std::span<Vertex>		Vertices;
	std::span<unsigned int> Indices;
};
