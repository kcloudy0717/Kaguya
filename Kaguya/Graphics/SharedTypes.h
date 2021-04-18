#pragma once
#include "SharedDefines.h"

namespace HLSL
{
	struct Material
	{
		int BSDFType;
		float3 baseColor;
		float metallic;
		float subsurface;
		float specular;
		float roughness;
		float specularTint;
		float anisotropic;
		float sheen;
		float sheenTint;
		float clearcoat;
		float clearcoatGloss;

		// Used by Glass BxDF
		float3 T;
		float etaA, etaB;

		int TextureIndices[NumTextureTypes];
		int TextureChannel[NumTextureTypes];
	};

	struct Light
	{
		uint Type;
		float3 Position;
		float4 Orientation;
		float Width;
		float Height;
		float3 Points[4]; // World-space points that are pre-computed on the Cpu so we don't have to compute them in shader for every ray

		float3 I;
	};

	struct Mesh
	{
		uint VertexOffset;
		uint IndexOffset;
		uint MaterialIndex;
		uint InstanceID : 24;
		uint InstanceMask : 8;
		uint InstanceContributionToHitGroupIndex : 24;
		uint Flags : 8;
		D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;
		matrix World;
		matrix PreviousWorld;
		float3x4 Transform;
	};

	struct Camera
	{
		float NearZ;
		float FarZ;
		float ExposureValue100;
		float _padding1;

		float FocalLength;
		float RelativeAperture;
		float ShutterTime;
		float SensorSensitivity;

		float4 Position;
		float4 U;
		float4 V;
		float4 W;

		matrix View;
		matrix Projection;
		matrix ViewProjection;
		matrix InvView;
		matrix InvProjection;
		matrix InvViewProjection;
	};
}