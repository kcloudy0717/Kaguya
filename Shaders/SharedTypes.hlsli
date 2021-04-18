#ifndef SHARED_TYPES_HLSLI
#define SHARED_TYPES_HLSLI

// ==================== Material ====================
enum TextureTypes
{
	AlbedoIdx,
	NormalIdx,
	RoughnessIdx,
	MetallicIdx,
	NumTextureTypes
};

#define TEXTURE_CHANNEL_RED		0
#define TEXTURE_CHANNEL_GREEN	1
#define TEXTURE_CHANNEL_BLUE	2
#define TEXTURE_CHANNEL_ALPHA	3

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

// ==================== Light ====================
#define LightType_Point (0)
#define LightType_Quad (1)

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

// ==================== Mesh ====================
struct Mesh
{
	uint		VertexOffset;
	uint		IndexOffset;
	uint		MaterialIndex;
	uint		InstanceIDAndMask;
	uint		InstanceContributionToHitGroupIndexAndFlags;
	uint64_t	AccelerationStructure;
	matrix		World;
	matrix		PreviousWorld;
	float3x4	Transform;
};

// ==================== Camera ====================
struct Camera
{
	float	NearZ;
	float	FarZ;
	float	ExposureValue100;
	float	_padding1;

	float	FocalLength;
	float	RelativeAperture;
	float	ShutterTime;
	float	SensorSensitivity;

	float4	Position;
	float4	U;
	float4	V;
	float4	W;

	matrix	View;
	matrix	Projection;
	matrix	ViewProjection;
	matrix	InvView;
	matrix	InvProjection;
	matrix	InvViewProjection;

	RayDesc GenerateCameraRay(in float2 ndc, inout uint seed)
	{
		float3 direction = ndc.x * U.xyz + ndc.y * V.xyz + W.xyz;

		// Find the focal point for this pixel
		direction /= length(W); // Make ray have length 1 along the camera's w-axis.
		float3 focalPoint = Position.xyz + FocalLength * direction; // Select point on ray a distance FocalLength along the w-axis

		// Get random numbers (in polar coordinates), convert to random cartesian uv on the lens
		float2 rnd = float2(g_2PI * RandomFloat01(seed), RelativeAperture * RandomFloat01(seed));
		float2 uv = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);

		// Use uv coordinate to compute a random origin on the camera lens
		float3 origin = Position.xyz + uv.x * normalize(U.xyz) + uv.y * normalize(V.xyz);
		direction = normalize(focalPoint - origin);

		RayDesc ray = { origin, NearZ, direction, FarZ };
		return ray;
	}
};

#endif // SHARED_TYPES_HLSLI