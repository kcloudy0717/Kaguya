cbuffer Settings : register(b0)
{
	float2 InverseOutputSize;
	float Intensity;
	uint InputIndex;
	uint BloomIndex;
	uint OutputIndex;
};
#include "../ShaderLayout.hlsli"

SamplerState LinearSampler : register(s0);

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	Texture2D Input = g_Texture2DTable[InputIndex];
	Texture2D Bloom = g_Texture2DTable[BloomIndex];
	RWTexture2D<float4> Output = g_RWTexture2DTable[OutputIndex];

	float2 UV = float2(DTid.xy + 0.5f) * InverseOutputSize;
	
	Output[DTid.xy] = Input[DTid.xy] + Intensity * Bloom.SampleLevel(LinearSampler, UV, 0.0f);
}