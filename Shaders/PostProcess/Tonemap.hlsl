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

float3 LinearTosRGB(float3 x)
{
	// Approximately pow(x, 1.0 / 2.2)
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

float3 sRGBToLinear(float3 x)
{
	return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
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
	Color.rgb = LinearTosRGB(Color.rgb);

	Output[Params.DispatchThreadID.xy] = float4(Color.rgb, 1.0f);
}
