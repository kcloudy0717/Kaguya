#include "../Shader.hlsli"
#include "../HlslDynamicResource.hlsli"

cbuffer Parameters : register(b0)
{
	float2 InverseOutputSize;
	uint   BloomIndex;
	uint   Output1Index;
	uint   Output2Index;
	uint   Output3Index;
	uint   Output4Index;
};

groupshared float4 g_Tile[64]; // 8x8 input pixels

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D			Bloom	= HLSL_TEXTURE2D(BloomIndex);
	RWTexture2D<float4> Output1 = HLSL_RWTEXTURE2D(Output1Index);
	RWTexture2D<float4> Output2 = HLSL_RWTEXTURE2D(Output2Index);
	RWTexture2D<float4> Output3 = HLSL_RWTEXTURE2D(Output3Index);
	RWTexture2D<float4> Output4 = HLSL_RWTEXTURE2D(Output4Index);

	// You can tell if both x and y are divisible by a power of two with this value
	uint parity = Params.DispatchThreadID.x | Params.DispatchThreadID.y;

	// Downsample and store the 8x8 block
	float2 centerUV						= (float2(Params.DispatchThreadID.xy) * 2.0f + 1.0f) * InverseOutputSize;
	float4 avgPixel						= Bloom.SampleLevel(g_SamplerLinearClamp, centerUV, 0.0f);
	g_Tile[Params.GroupIndex]			= avgPixel;
	Output1[Params.DispatchThreadID.xy] = avgPixel;

	GroupMemoryBarrierWithGroupSync();

	// Downsample and store the 4x4 block
	if ((parity & 1) == 0)
	{
		avgPixel								 = 0.25f * (avgPixel + g_Tile[Params.GroupIndex + 1] + g_Tile[Params.GroupIndex + 8] + g_Tile[Params.GroupIndex + 9]);
		g_Tile[Params.GroupIndex]				 = avgPixel;
		Output2[Params.DispatchThreadID.xy >> 1] = avgPixel;
	}

	GroupMemoryBarrierWithGroupSync();

	// Downsample and store the 2x2 block
	if ((parity & 3) == 0)
	{
		avgPixel								 = 0.25f * (avgPixel + g_Tile[Params.GroupIndex + 2] + g_Tile[Params.GroupIndex + 16] + g_Tile[Params.GroupIndex + 18]);
		g_Tile[Params.GroupIndex]				 = avgPixel;
		Output3[Params.DispatchThreadID.xy >> 2] = avgPixel;
	}

	GroupMemoryBarrierWithGroupSync();

	// Downsample and store the 1x1 block
	if ((parity & 7) == 0)
	{
		avgPixel								 = 0.25f * (avgPixel + g_Tile[Params.GroupIndex + 4] + g_Tile[Params.GroupIndex + 32] + g_Tile[Params.GroupIndex + 36]);
		Output4[Params.DispatchThreadID.xy >> 3] = avgPixel;
	}
}