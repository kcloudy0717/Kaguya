#pragma once

#include "Math.hlsli"

// Raytracing Gems 2

#define USE_PCG 1
#if USE_PCG
#define RngStateType uint4
#else
#define RngStateType uint
#endif

// PCG random numbers generator
// Source: "Hash Functions for GPU Rendering" by Jarzynski & Olano
uint4 pcg4d(uint4 v)
{
	v = v * 1664525u + 1013904223u;

	v.x += v.y * v.w;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v.w += v.y * v.z;

	v = v ^ (v >> 16u);

	v.x += v.y * v.w;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v.w += v.y * v.z;

	return v;
}

// 32-bit Xorshift random number generator
uint xorshift(inout uint rngState)
{
	rngState ^= rngState << 13;
	rngState ^= rngState >> 17;
	rngState ^= rngState << 5;
	return rngState;
}

// Jenkins's "one at a time" hash function
uint jenkinsHash(uint x)
{
	x += x << 10;
	x ^= x >> 6;
	x += x << 3;
	x ^= x >> 11;
	x += x << 15;
	return x;
}

// Converts unsigned integer into float int range <0; 1) by using 23 most significant bits for mantissa
float uintToFloat(uint x)
{
	return asfloat(0x3f800000 | (x >> 9)) - 1.0f;
}

#if USE_PCG

// Initialize RNG for given pixel, and frame number (PCG version)
RngStateType initRNG(uint2 pixelCoords, uint2 resolution, uint frameNumber)
{
	// Seed for PCG uses a sequential sample number in 4th channel, which increments on every RNG call and starts from 0
	return RngStateType(pixelCoords.xy, frameNumber, 0);
}

// Return random float in <0; 1) range  (PCG version)
float rand(inout RngStateType rngState)
{
	rngState.w++; //< Increment sample index
	return uintToFloat(pcg4d(rngState).x);
}

#else

// Initialize RNG for given pixel, and frame number (Xorshift-based version)
RngStateType initRNG(uint2 pixelCoords, uint2 resolution, uint frameNumber)
{
	RngStateType seed = dot(pixelCoords, uint2(1, resolution.x)) ^ jenkinsHash(frameNumber);
	return jenkinsHash(seed);
}

// Return random float in <0; 1) range (Xorshift-based version)
float rand(inout RngStateType rngState)
{
	return uintToFloat(xorshift(rngState));
}

#endif
