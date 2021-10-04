#include "ACES.hlsli"

#include "DescriptorTable.hlsli"
#include "FullScreenTriangle.hlsl"

cbuffer Settings : register(b0, space0)
{
	uint InputIndex;
};

float4 PSMain(VSOutput IN) : SV_TARGET
{
	// Texture2D Input = g_Texture2DTable[InputIndex];
	Texture2D Input = ResourceDescriptorHeap[InputIndex];
	// Texture2D Input = ResourceDescriptorHeap[2];

	float3 color = Input.Sample(g_SamplerPointClamp, IN.Texture).rgb;

	color = ACESFitted(color);
	return float4(color, 1.0f);
}