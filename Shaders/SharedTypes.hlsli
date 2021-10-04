#pragma once
#include "Math.hlsli"

// ==================== Material ====================
struct Material
{
	uint   BSDFType;
	float3 baseColor;
	float  metallic;
	float  subsurface;
	float  specular;
	float  roughness;
	float  specularTint;
	float  anisotropic;
	float  sheen;
	float  sheenTint;
	float  clearcoat;
	float  clearcoatGloss;

	// Used by Glass BxDF
	float3 T;
	float  etaA, etaB;

	// Texture indices
	int Albedo;
};

// ==================== Light ====================
#define LightType_Point (0)
#define LightType_Quad	(1)

struct Light
{
	uint   Type;
	float3 Position;
	float4 Orientation;
	float  Width;
	float  Height;
	float3 Points[4]; // World-space points that are pre-computed on the Cpu so we don't have to compute them in shader
					  // for every ray

	float3 I;
};

// ==================== Mesh ====================
struct Mesh
{
	matrix Transform;
	matrix PreviousTransform;

	BoundingBox BoundingBox;

	unsigned int MaterialIndex;
};

// ==================== Camera ====================
struct Camera
{
	float FoVY; // Degrees
	float AspectRatio;
	float NearZ;
	float FarZ;

	float FocalLength;
	float RelativeAperture;
	float DEADBEEF0;
	float DEADBEEF1;

	float4 Position;

	matrix View;
	matrix Projection;
	matrix ViewProjection;

	matrix InvView;
	matrix InvProjection;
	matrix InvViewProjection;

	matrix PrevViewProjection;

	Frustum Frustum;

	RayDesc GenerateCameraRay(float2 ndc)
	{
		// Setup the ray
		RayDesc ray;
		ray.Origin = InvView[3].xyz;
		ray.TMin   = NearZ;
		ray.TMax   = FarZ;

		// Extract the aspect ratio and field of view from the projection matrix
		float tanHalfFoVY = tan(radians(FoVY) * 0.5f);

		// Compute the ray direction for this pixel
		float3 right   = ndc.x * InvView[0].xyz * tanHalfFoVY * AspectRatio;
		float3 up	   = ndc.y * InvView[1].xyz * tanHalfFoVY;
		float3 forward = InvView[2].xyz;
		ray.Direction  = normalize(right + up + forward);

		return ray;
	}
};
