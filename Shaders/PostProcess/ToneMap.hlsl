#include "ACES.hlsli"
#include "GranTurismoOperator.hlsli"

#include <DescriptorTable.hlsli>

#include <FullScreenTriangle.hlsl>

cbuffer Settings : register(b0, space0)
{
	uint InputIndex;
};

SamplerState PointClamp : register(s0, space0);

float3 LinearTosRGB(float3 u)
{
	return u <= 0.0031308 ? 12.92 * u : 1.055 * pow(u, 1.0 / 2.4) - 0.055;
}

float3 sRGBToLinear(float3 u)
{
	return u <= 0.04045 ? u / 12.92 : pow((u + 0.055) / 1.055, 2.4);
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D Input = g_Texture2DTable[InputIndex];
	
	float3 color = Input.Sample(PointClamp, IN.Texture).rgb;
	
	color = ACESFitted(color);
	return float4(color, 1.0f);
}