#include "Shader.hlsli"
#include "HlslDynamicResource.hlsli"

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
	Texture2D			Input  = HLSL_TEXTURE2D(InputIndex);
	Texture2D			Bloom  = HLSL_TEXTURE2D(BloomIndex);
	RWTexture2D<float4> Output = HLSL_RWTEXTURE2D(OutputIndex);

	// Bloom
	float2 UV	 = float2(Params.DispatchThreadID.xy + 0.5f) * InverseOutputSize;
	float4 Color = Input[Params.DispatchThreadID.xy];
	Color += BloomIntensity * Bloom.SampleLevel(g_SamplerLinearClamp, UV, 0.0f);

	// Tonemap
	Color.rgb = ACESFitted(Color.rgb);
	// Convert linear to sRGB
	Color.rgb = LinearTosRGB(Color.rgb);

	Output[Params.DispatchThreadID.xy] = float4(Color.rgb, 1.0f);
}
