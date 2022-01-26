#include "../Shader.hlsli"
#include "../HlslDynamicResource.hlsli"

cbuffer Parameters : register(b0)
{
	uint InputIndex;
	uint OutputIndex;
}

// The guassian blur weights (derived from Pascal's triangle)
static const float Weights[5] = { 70.0f / 256.0f, 56.0f / 256.0f, 28.0f / 256.0f, 8.0f / 256.0f, 1.0f / 256.0f };

float3 BlurPixels(float3 a, float3 b, float3 c, float3 d, float3 e, float3 f, float3 g, float3 h, float3 i)
{
	return Weights[0] * e + Weights[1] * (d + f) + Weights[2] * (c + g) + Weights[3] * (b + h) + Weights[4] * (a + i);
}

// 16x16 pixels with an 8x8 center that we will be blurring writing out.  Each uint is two color channels packed together
groupshared uint CacheR[128];
groupshared uint CacheG[128];
groupshared uint CacheB[128];

void Store2Pixels(uint index, float3 pixel1, float3 pixel2)
{
	CacheR[index] = f32tof16(pixel1.r) | f32tof16(pixel2.r) << 16;
	CacheG[index] = f32tof16(pixel1.g) | f32tof16(pixel2.g) << 16;
	CacheB[index] = f32tof16(pixel1.b) | f32tof16(pixel2.b) << 16;
}

void Load2Pixels(uint index, out float3 pixel1, out float3 pixel2)
{
	uint rr = CacheR[index];
	uint gg = CacheG[index];
	uint bb = CacheB[index];
	pixel1	= float3(f16tof32(rr), f16tof32(gg), f16tof32(bb));
	pixel2	= float3(f16tof32(rr >> 16), f16tof32(gg >> 16), f16tof32(bb >> 16));
}

void Store1Pixel(uint index, float3 pixel)
{
	CacheR[index] = asuint(pixel.r);
	CacheG[index] = asuint(pixel.g);
	CacheB[index] = asuint(pixel.b);
}

void Load1Pixel(uint index, out float3 pixel)
{
	pixel = asfloat(uint3(CacheR[index], CacheG[index], CacheB[index]));
}

// Blur two pixels horizontally.  This reduces LDS reads and pixel unpacking.
void BlurHorizontally(uint outIndex, uint leftMostIndex)
{
	float3 s0, s1, s2, s3, s4, s5, s6, s7, s8, s9;
	Load2Pixels(leftMostIndex + 0, s0, s1);
	Load2Pixels(leftMostIndex + 1, s2, s3);
	Load2Pixels(leftMostIndex + 2, s4, s5);
	Load2Pixels(leftMostIndex + 3, s6, s7);
	Load2Pixels(leftMostIndex + 4, s8, s9);

	Store1Pixel(outIndex, BlurPixels(s0, s1, s2, s3, s4, s5, s6, s7, s8));
	Store1Pixel(outIndex + 1, BlurPixels(s1, s2, s3, s4, s5, s6, s7, s8, s9));
}

void BlurVertically(uint2 pixelCoord, uint topMostIndex, RWTexture2D<float4> output)
{
	float3 s0, s1, s2, s3, s4, s5, s6, s7, s8;
	Load1Pixel(topMostIndex, s0);
	Load1Pixel(topMostIndex + 8, s1);
	Load1Pixel(topMostIndex + 16, s2);
	Load1Pixel(topMostIndex + 24, s3);
	Load1Pixel(topMostIndex + 32, s4);
	Load1Pixel(topMostIndex + 40, s5);
	Load1Pixel(topMostIndex + 48, s6);
	Load1Pixel(topMostIndex + 56, s7);
	Load1Pixel(topMostIndex + 64, s8);

	output[pixelCoord] = float4(BlurPixels(s0, s1, s2, s3, s4, s5, s6, s7, s8), 1.0f);
}

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	Texture2D			Input  = HLSL_TEXTURE2D(InputIndex);
	RWTexture2D<float4> Output = HLSL_RWTEXTURE2D(OutputIndex);

	//
	// Load 4 pixels per thread into LDS
	//
	int2 GroupUL  = (Params.GroupID.xy << 3) - 4;			  // Upper-left pixel coordinate of group read location
	int2 ThreadUL = (Params.GroupThreadID.xy << 1) + GroupUL; // Upper-left pixel coordinate of quad that this thread will read

	//
	// Store 4 unblurred pixels in LDS
	//
	int destIdx = Params.GroupThreadID.x + (Params.GroupThreadID.y << 4);
	Store2Pixels(destIdx + 0, Input[ThreadUL + uint2(0, 0)].rgb, Input[ThreadUL + uint2(1, 0)].rgb);
	Store2Pixels(destIdx + 8, Input[ThreadUL + uint2(0, 1)].rgb, Input[ThreadUL + uint2(1, 1)].rgb);

	GroupMemoryBarrierWithGroupSync();

	//
	// Horizontally blur the pixels in Cache
	//
	uint row = Params.GroupThreadID.y << 4;
	BlurHorizontally(row + (Params.GroupThreadID.x << 1), row + Params.GroupThreadID.x + (Params.GroupThreadID.x & 4));

	GroupMemoryBarrierWithGroupSync();

	//
	// Vertically blur the pixels and write the result to memory
	//
	BlurVertically(Params.DispatchThreadID.xy, (Params.GroupThreadID.y << 3) + Params.GroupThreadID.x, Output);
}
