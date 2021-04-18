#ifndef VERTEX_HLSLI
#define VERTEX_HLSLI

struct Vertex
{
	float3 Position;
	float2 TextureCoordinate;
	float3 Normal;
};

float BarycentricInterpolation(float v0, float v1, float v2, float3 barycentric)
{
	return v0 * barycentric.x + v1 * barycentric.y + v2 * barycentric.z;
}

float2 BarycentricInterpolation(float2 v0, float2 v1, float2 v2, float3 barycentric)
{
	return v0 * barycentric.x + v1 * barycentric.y + v2 * barycentric.z;
}

float3 BarycentricInterpolation(float3 v0, float3 v1, float3 v2, float3 barycentric)
{
	return v0 * barycentric.x + v1 * barycentric.y + v2 * barycentric.z;
}

Vertex BarycentricInterpolation(Vertex v0, Vertex v1, Vertex v2, float3 barycentric)
{
	Vertex vertex;
	vertex.Position = BarycentricInterpolation(v0.Position, v1.Position, v2.Position, barycentric);
	vertex.TextureCoordinate = BarycentricInterpolation(v0.TextureCoordinate, v1.TextureCoordinate, v2.TextureCoordinate, barycentric);
	vertex.Normal = BarycentricInterpolation(v0.Normal, v1.Normal, v2.Normal, barycentric);

	return vertex;
}

#endif