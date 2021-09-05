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

[[vk::binding(0, 0)]] Texture2D Texture2DTable[];
[[vk::binding(1, 0)]] RWTexture2D<float4> RWTexture2DTable[];

[[vk::binding(0, 1)]] SamplerState Sampler;

[[vk::binding(0, 2)]] ConstantBuffer<UniformSceneConstants> SceneConstants;

float4 main(VSOutput Input)
	: SV_TARGET
{
	Texture2D t = Texture2DTable[PushConstants.Id1];
	return t.Sample(Sampler, Input.TextureCoord);
}
