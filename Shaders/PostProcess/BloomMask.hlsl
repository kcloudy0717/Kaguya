#include "../Constants.hlsli"

cbuffer Settings : register(b0)
{
	float2 InverseOutputSize;
	float Threshold;
	uint InputIndex;
	uint OutputIndex;
};

#include "../ShaderLayout.hlsli"

SamplerState BiLinearClamp : register(s0);

// This assumes the default color gamut found in sRGB and REC709.  The color primaries determine these
// coefficients.  Note that this operates on linear values, not gamma space.
float RGBToLuminance(float3 x)
{
	return dot(x, float3(0.212671f, 0.715160f, 0.072169f)); // Defined by sRGB/Rec.709 gamut
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	Texture2D Input = g_Texture2DTable[InputIndex];
	RWTexture2D<float4> Output = g_RWTexture2DTable[OutputIndex];

	// We need the scale factor and the size of one pixel so that our four samples are right in the middle
    // of the quadrant they are covering.
	float2 uv = (DTid.xy + 0.5f) * InverseOutputSize;
	float2 offset = InverseOutputSize * 0.25f;

    // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
	float3 color1 = Input.SampleLevel(BiLinearClamp, uv + float2(-offset.x, -offset.y), 0.0f).rgb;
	float3 color2 = Input.SampleLevel(BiLinearClamp, uv + float2(+offset.x, -offset.y), 0.0f).rgb;
	float3 color3 = Input.SampleLevel(BiLinearClamp, uv + float2(-offset.x, +offset.y), 0.0f).rgb;
	float3 color4 = Input.SampleLevel(BiLinearClamp, uv + float2(+offset.x, +offset.y), 0.0f).rgb;

	float luma1 = RGBToLuminance(color1);
	float luma2 = RGBToLuminance(color2);
	float luma3 = RGBToLuminance(color3);
	float luma4 = RGBToLuminance(color4);

	const float kSmallEpsilon = 0.0001f;

    // We perform a brightness filter pass, where lone bright pixels will contribute less.
	color1 *= max(kSmallEpsilon, luma1 - Threshold) / (luma1 + kSmallEpsilon);
	color2 *= max(kSmallEpsilon, luma2 - Threshold) / (luma2 + kSmallEpsilon);
	color3 *= max(kSmallEpsilon, luma3 - Threshold) / (luma3 + kSmallEpsilon);
	color4 *= max(kSmallEpsilon, luma4 - Threshold) / (luma4 + kSmallEpsilon);

    // The shimmer filter helps remove stray bright pixels from the bloom buffer by inversely weighting
    // them by their luminance.  The overall effect is to shrink bright pixel regions around the border.
    // Lone pixels are likely to dissolve completely.  This effect can be tuned by adjusting the shimmer
    // filter inverse strength.  The bigger it is, the less a pixel's luminance will matter.
	const float kShimmerFilterInverseStrength = 1.0f;
	float weight1 = 1.0f / (luma1 + kShimmerFilterInverseStrength);
	float weight2 = 1.0f / (luma2 + kShimmerFilterInverseStrength);
	float weight3 = 1.0f / (luma3 + kShimmerFilterInverseStrength);
	float weight4 = 1.0f / (luma4 + kShimmerFilterInverseStrength);
	float weightSum = weight1 + weight2 + weight3 + weight4;
	
	float4 result = float4((color1 * weight1 + color2 * weight2 + color3 * weight3 + color4 * weight4) / weightSum, 0.0f);
	if (!any(isnan(result)))
		Output[DTid.xy] = result;
}