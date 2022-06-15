#pragma once

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it.

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat = { { 0.59719, 0.35458, 0.04823 },
									   { 0.07600, 0.90834, 0.01566 },
									   { 0.02840, 0.13383, 0.83777 } };

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat = { { 1.60475, -0.53108, -0.07367 },
										{ -0.10208, 1.10813, -0.00605 },
										{ -0.00327, -0.07276, 1.07602 } };

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

// This is a more complex fit than the ACESFilm
float3 ACESFitted(float3 color)
{
	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(ACESOutputMat, color);

	// Clamp to [0, 1]
	return saturate(color);
}

// This is a very simple luminance only fit, which over saturates brights.
float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}
