struct Vertex
{
	[[vk::location(0)]] float3 Position : POSITION;
	[[vk::location(1)]] float2 TextureCoord : TEXCOORD;
	[[vk::location(2)]] float3 Normal : NORMAL;
};

struct VSOutput
{
	float4					   Pos : SV_POSITION;
	[[vk::location(0)]] float3 Color : COLOR;
};

struct MeshPushConstants
{
	float4	 data;
	float4x4 render_matrix;
};
[[vk::push_constant]] MeshPushConstants PushConstants;

VSOutput main(Vertex Vertex)
{
	VSOutput output = (VSOutput)0;
	output.Pos		= mul(float4(Vertex.Position, 1.0f), PushConstants.render_matrix);
	output.Color	= Vertex.Normal;
	return output;
}
