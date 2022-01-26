#include "Shader.hlsli"
#include "HlslDynamicResource.hlsli"

cbuffer Parameters : register(b0)
{
	uint InputIndex;
	uint OutputIndex;
};

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	static const uint Dimension			 = 8;
	static const uint BayerMatrix8x8[64] =
	{
		 0, 32,  8, 40,  2, 34, 10, 42,
		48, 16, 56, 24, 50, 18, 58, 26,
		12, 44,  4, 36, 14, 46,  6, 38,
		60, 28, 52, 20, 62, 30, 54, 22,
		 3, 35, 11, 43,  1, 33,  9, 41,
		51, 19, 59, 27, 49, 17, 57, 25,
		15, 47,  7, 39, 13, 45,  5, 37,
		63, 31, 55, 23, 61, 29, 53, 21
	};

	Texture2D			Input  = HLSL_TEXTURE2D(InputIndex);
	RWTexture2D<float4> Output = HLSL_RWTEXTURE2D(OutputIndex);

	uint  x			 = Params.DispatchThreadID.x % Dimension;
	uint  y			 = Params.DispatchThreadID.y % Dimension;
	float BayerValue = BayerMatrix8x8[y * Dimension + x] / 64.0f;

	float4 Color = Input[Params.DispatchThreadID.xy];
	Color.r		 = step(BayerValue, Color.r);
	Color.g		 = step(BayerValue, Color.g);
	Color.b		 = step(BayerValue, Color.b);

	Output[Params.DispatchThreadID.xy] = float4(Color.rgb, 1.0f);
}
