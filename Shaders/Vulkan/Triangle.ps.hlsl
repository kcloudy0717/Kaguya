struct VSOutput
{
	float4					   Pos : SV_POSITION;
	[[vk::location(0)]] float2 TextureCoord : TEXCOORD;
	[[vk::location(1)]] float3 Normal : NORMAL;
};

struct MeshPushConstants
{
	float4x4 Transform;
	uint	 Id;
};
[[vk::push_constant]] MeshPushConstants PushConstants;

struct UniformSceneConstants
{
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
};
[[vk::binding(0, 0)]] ConstantBuffer<UniformSceneConstants> SceneConstants : register(b0);

[[vk::binding(1, 0)]] Texture2D			  Texture2DTable[] : register(t0, space100);
[[vk::binding(2, 0)]] RWTexture2D<float4> RWTexture2DTable[] : register(u0, space101);
[[vk::binding(3, 0)]] SamplerState		  Sampler : register(s0, space102);

float4 main(VSOutput Input) : SV_TARGET
{
    Texture2D t = Texture2DTable[PushConstants.Id];
    return t.Sample(Sampler, Input.TextureCoord);
}
