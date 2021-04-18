cbuffer Settings : register(b0)
{
	float2 InverseOutputSize;
	uint BloomIndex;
	uint Output1Index;
	uint Output2Index;
	uint Output3Index;
	uint Output4Index;
};

#include "../ShaderLayout.hlsli"

SamplerState BiLinearClamp : register(s0);

groupshared float4 g_Tile[64]; // 8x8 input pixels

[numthreads(8, 8, 1)]
void CSMain(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
	Texture2D Bloom = g_Texture2DTable[BloomIndex];
	RWTexture2D<float4> Output1 = g_RWTexture2DTable[Output1Index];
	RWTexture2D<float4> Output2 = g_RWTexture2DTable[Output2Index];
	RWTexture2D<float4> Output3 = g_RWTexture2DTable[Output3Index];
	RWTexture2D<float4> Output4 = g_RWTexture2DTable[Output4Index];

    // You can tell if both x and y are divisible by a power of two with this value
	uint parity = DTid.x | DTid.y;

    // Downsample and store the 8x8 block
	float2 centerUV = (float2(DTid.xy) * 2.0f + 1.0f) * InverseOutputSize;
	float4 avgPixel = Bloom.SampleLevel(BiLinearClamp, centerUV, 0.0f);
	g_Tile[GI] = avgPixel;
	Output1[DTid.xy] = avgPixel;

	GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 4x4 block
	if ((parity & 1) == 0)
	{
		avgPixel = 0.25f * (avgPixel + g_Tile[GI + 1] + g_Tile[GI + 8] + g_Tile[GI + 9]);
		g_Tile[GI] = avgPixel;
		Output2[DTid.xy >> 1] = avgPixel;
	}

	GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 2x2 block
	if ((parity & 3) == 0)
	{
		avgPixel = 0.25f * (avgPixel + g_Tile[GI + 2] + g_Tile[GI + 16] + g_Tile[GI + 18]);
		g_Tile[GI] = avgPixel;
		Output3[DTid.xy >> 2] = avgPixel;
	}

	GroupMemoryBarrierWithGroupSync();

    // Downsample and store the 1x1 block
	if ((parity & 7) == 0)
	{
		avgPixel = 0.25f * (avgPixel + g_Tile[GI + 4] + g_Tile[GI + 32] + g_Tile[GI + 36]);
		Output4[DTid.xy >> 3] = avgPixel;
	}
}