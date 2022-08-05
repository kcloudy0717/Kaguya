#include "../Common/ThreadGroupIDSwizzling.hlsli"
#include "../Shader.hlsli"
#include "../HlslDynamicResource.hlsli"

cbuffer Parameters : register(b0)
{
	uint  OutputIndex;
	uint2 DispatchArgument;
};

[numthreads(8, 8, 1)]
void CSMain(CSParams Params)
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

	RWTexture2D<float4> Output = ResourceDescriptorHeap[OutputIndex];

	uint2 Coordinate = ThreadGroupTilingX(DispatchArgument, uint2(8, 8), 8, Params.GroupThreadID.xy, Params.GroupID.xy);
	// uint2 Coordinate = Params.DispatchThreadID.xy;

	uint  x			 = Coordinate.x % Dimension;
	uint  y			 = Coordinate.y % Dimension;
	float BayerValue = BayerMatrix8x8[y * Dimension + x] / 64.0f;

	float4 Color	   = Output[Coordinate];
	Color.r			   = step(BayerValue, Color.r);
	Color.g			   = step(BayerValue, Color.g);
	Color.b			   = step(BayerValue, Color.b);
	Output[Coordinate] = float4(Color.rgb, 1.0f);
}
