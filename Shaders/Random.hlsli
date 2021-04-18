#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

#include <Math.hlsli>

uint WangHash(inout uint seed)
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

float RandomFloat01(inout uint seed)
{
	return float(WangHash(seed)) / 4294967296.0;
}

#endif // RANDOM_HLSLI