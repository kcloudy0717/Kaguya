#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "DescriptorTable.hlsli"

#include "ACES.hlsli"

cbuffer Parameters : register(b0)
{
	float2 InverseOutputSize;
	float  BloomIntensity;
	uint   InputIndex;
	uint   BloomIndex;
	uint   OutputIndex;
};

float3 LinearTosRGB(float3 x)
{
	// Approximately pow(x, 1.0 / 2.2)
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D			Input  = g_Texture2DTable[InputIndex];
	Texture2D			Bloom  = g_Texture2DTable[BloomIndex];
	RWTexture2D<float4> Output = g_RWTexture2DTable[OutputIndex];

	// Bloom
	float2 UV = float2(Params.DispatchThreadID.xy + 0.5f) * InverseOutputSize;
	float4 Color = Input[Params.DispatchThreadID.xy];
	Color += BloomIntensity * Bloom.SampleLevel(g_SamplerLinearClamp, UV, 0.0f);

	// Tonemap
	Color.rgb = ACESFitted(Color.rgb);
	// Convert linear to sRGB
	Color.rgb = LinearTosRGB(Color.rgb);

	Output[Params.DispatchThreadID.xy] = float4(Color.rgb, 1.0f);
}
