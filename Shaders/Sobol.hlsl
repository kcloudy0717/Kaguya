#include "Shader.hlsli"
#include "HlslDynamicResource.hlsli"

cbuffer Parameters : register(b0)
{
	uint InputIndex;
	uint OutputIndex;
};

// Approximates luminance ("brightness") from an RGB value.  These weights are derived from
// experiment based on eye sensitivity to different wavelengths of light.
float CalcLuminance(float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D			Input  = HLSL_TEXTURE2D(InputIndex);
	RWTexture2D<float4> Output = HLSL_RWTEXTURE2D(OutputIndex);

	// Sample the pixels in the neighborhood of this pixel.
	float4 c[3][3];
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			int2 xy = Params.DispatchThreadID.xy + int2(-1 + j, -1 + i);
			c[i][j] = Input[xy];
		}
	}

	// For each color channel, estimate partial x derivative using Sobel scheme.
	float4 Gx = -1.0f * c[0][0] - 2.0f * c[1][0] - 1.0f * c[2][0] + 1.0f * c[0][2] + 2.0f * c[1][2] + 1.0f * c[2][2];
	// For each color channel, estimate partial y derivative using Sobel scheme.
	float4 Gy = -1.0f * c[2][0] - 2.0f * c[2][1] - 1.0f * c[2][1] + 1.0f * c[0][0] + 2.0f * c[0][1] + 1.0f * c[0][2];

	// Gradient is (Gx, Gy).  For each color channel, compute magnitude to get maximum rate of change.
	float4 mag = sqrt(Gx * Gx + Gy * Gy);

	// Make edges black, and nonedges white.
	mag = 1.0f - saturate(CalcLuminance(mag.rgb));

	Output[Params.DispatchThreadID.xy] = mag * Input[Params.DispatchThreadID.xy];
}
