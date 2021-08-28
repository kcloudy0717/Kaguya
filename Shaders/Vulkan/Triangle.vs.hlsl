struct Vertex
{
	[[vk::location(0)]] float3 Position : POSITION;
	[[vk::location(1)]] float2 TextureCoord : TEXCOORD;
	[[vk::location(2)]] float3 Normal : NORMAL;
};

struct VSOutput
{
	float4					   Pos : SV_POSITION;
	[[vk::location(0)]] float2 TextureCoord : TEXCOORD;
	[[vk::location(1)]] float3 Normal : NORMAL;
};

struct MeshPushConstants
{
	float4x4 Transform;
	uint	 Id1;
	uint	 Id2;
};
[[vk::push_constant]] MeshPushConstants PushConstants;

struct UniformSceneConstants
{
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
};
[[vk::binding(0, 0)]] ConstantBuffer<UniformSceneConstants> SceneConstants;

[[vk::binding(1, 0)]] Texture2D			  Texture2DTable[];
[[vk::binding(2, 0)]] RWTexture2D<float4> RWTexture2DTable[];

[[vk::binding(0, 1)]] SamplerState Sampler;

VSOutput main(Vertex Vertex)
{
	VSOutput output		= (VSOutput)0;
	output.Pos			= mul(float4(Vertex.Position, 1.0f), PushConstants.Transform);
	output.Pos			= mul(float4(output.Pos.xyz, 1.0f), SceneConstants.ViewProjection);
	output.TextureCoord = Vertex.TextureCoord;
	output.Normal		= Vertex.Normal;
	return output;
}
