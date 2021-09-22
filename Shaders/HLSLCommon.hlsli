#ifndef HLSL_COMMON_HLSLI
#define HLSL_COMMON_HLSLI

#include "Vertex.hlsli"
#include "Math.hlsli"
#include "Sampling.hlsli"
#include "Random.hlsli"
#include "BSDF.hlsli"
#include "SharedTypes.hlsli"

float3 CartesianToSpherical(float x, float y, float z)
{
	float radius = sqrt(x * x + y * y + z * z);
	float theta	 = acos(z / radius);
	float phi	 = atan(y / x);
	return float3(radius, theta, phi);
}

float3 SphericalToCartesian(float Radius, float Theta, float Phi)
{
	float x = Radius * sin(Theta) * cos(Phi);
	float y = Radius * sin(Theta) * sin(Phi);
	float z = Radius * cos(Theta);
	return float3(x, y, z);
}

// Returns a relative luminance of an input linear RGB color in the Rec. 709 color space
float RGBToCIELuminance(float3 RGB)
{
	return dot(RGB, float3(0.212671f, 0.715160f, 0.072169f));
}

float NDCDepthToViewDepth(float NDCDepth, Camera Camera)
{
	return Camera.Projection[3][2] / (NDCDepth - Camera.Projection[2][2]);
}

// Reconstructs view position from depth buffer
float3 NDCDepthToViewPosition(float NDCDepth, float2 ScreenSpaceUV, Camera Camera)
{
	ScreenSpaceUV.y = 1.0f - ScreenSpaceUV.y; // Flip due to DirectX convention

	float  z  = NDCDepth;
	float2 xy = ScreenSpaceUV * 2.0f - 1.0f;

	float4 clipSpacePosition = float4(xy, z, 1.0f);
	float4 viewSpacePosition = mul(clipSpacePosition, Camera.InvProjection);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}

// Reconstructs world position from depth buffer
float3 NDCDepthToWorldPosition(float NDCDepth, float2 ScreenSpaceUV, Camera Camera)
{
	float4 viewSpacePosition  = float4(NDCDepthToViewPosition(NDCDepth, ScreenSpaceUV, Camera), 1.0f);
	float4 worldSpacePosition = mul(viewSpacePosition, Camera.InvView);
	return worldSpacePosition.xyz;
}

float3 GenerateWorldCameraRayDirection(float2 ScreenSpaceUV, Camera Camera)
{
	return normalize(NDCDepthToWorldPosition(1.0f, ScreenSpaceUV, Camera) - Camera.Position.xyz);
}

#endif // HLSL_COMMON_HLSLI
