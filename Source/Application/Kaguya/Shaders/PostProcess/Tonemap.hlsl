#include "../Shader.hlsli"
#include "../HlslDynamicResource.hlsli"

#include "ACES.hlsli"

cbuffer Parameters : register(b0)
{
	float2 InverseOutputSize;
	float  BloomIntensity;
	uint   InputIndex;
	uint   BloomIndex;
	uint   OutputIndex;
};

float sRGB_OETF(float x)
{
	return x <= 0.0031308f ? 12.92f * x : 1.055f * pow(x, 1.0f / 2.4f) - 0.055f;
}

float3 sRGB_OETF(float3 x)
{
	return float3(sRGB_OETF(x.x), sRGB_OETF(x.y), sRGB_OETF(x.z));
}

float sRGB_EOTF(float x)
{
	return x <= 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

float3 sRGB_EOTF(float3 x)
{
	return float3(sRGB_EOTF(x.x), sRGB_EOTF(x.y), sRGB_EOTF(x.z));
}

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D			Input  = ResourceDescriptorHeap[InputIndex];
	RWTexture2D<float4> Output = ResourceDescriptorHeap[OutputIndex];

	float4 Color = Input[Params.DispatchThreadID.xy];

	// Bloom
	if (BloomIndex != -1)
	{
		Texture2D Bloom = ResourceDescriptorHeap[BloomIndex];
		float2	  UV	= float2(Params.DispatchThreadID.xy + 0.5f) * InverseOutputSize;
		Color += BloomIntensity * Bloom.SampleLevel(g_SamplerLinearClamp, UV, 0.0f);
	}

	// Tonemap
	Color.rgb = ACESFitted(Color.rgb);
	// Convert linear to sRGB
	Color.rgb = sRGB_OETF(Color.rgb);

	Output[Params.DispatchThreadID.xy] = float4(Color.rgb, 1.0f);
}
