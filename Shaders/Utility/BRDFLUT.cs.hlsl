#include "../Shader.hlsli"
#include "../HlslDynamicResource.hlsli"
#include "../HLSLCommon.hlsli"

cbuffer Parameters : register(b0)
{
	uint OutputIndex;
};

// based on http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float Random(float2 co)
{
	float a	 = 12.9898;
	float b	 = 78.233;
	float c	 = 43758.5453;
	float dt = dot(co.xy, float2(a, b));
	float sn = dt % g_PI;
	return frac(sin(sn) * c);
}

float2 Hammersley2d(uint i, uint N)
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits	  = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits	  = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits	  = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits	  = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) / float(N), rdi);
}

// based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 normal)
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float  alpha	= roughness * roughness;
	float  phi		= 2.0 * g_PI * Xi.x + Random(normal.xz) * 0.1;
	float  cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float  sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	float3 H		= float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	float3 up		= abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangentX = normalize(cross(up, normal));
	float3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Geometric Shadowing function
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float k	 = (roughness * roughness) / 2.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

float2 BRDF(float NoV, float roughness)
{
	// Normal always points along z-axis for the 2D lookup
	const float3 N = float3(0.0, 0.0, 1.0);
	float3		 V = float3(sqrt(1.0 - NoV * NoV), 0.0, NoV);

	float2	   LUT		   = float2(0.0f, 0.0f);
	const uint NUM_SAMPLES = 1024;
	for (uint i = 0u; i < NUM_SAMPLES; i++)
	{
		float2 Xi = Hammersley2d(i, NUM_SAMPLES);
		float3 H  = ImportanceSampleGGX(Xi, roughness, N);
		float3 L  = 2.0 * dot(V, H) * H - V;

		float dotNL = max(dot(N, L), 0.0);
		float dotNV = max(dot(N, V), 0.0);
		float dotVH = max(dot(V, H), 0.0);
		float dotNH = max(dot(H, N), 0.0);

		if (dotNL > 0.0)
		{
			float G		= G_SchlicksmithGGX(dotNL, dotNV, roughness);
			float G_Vis = (G * dotVH) / (dotNH * dotNV);
			float Fc	= pow(1.0 - dotVH, 5.0);
			LUT += float2((1.0 - Fc) * G_Vis, Fc * G_Vis);
		}
	}
	return LUT / float(NUM_SAMPLES);
}

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	const uint Width  = 256;
	const uint Height = 256;

	float2 uv;
	uv.x = (float(Params.DispatchThreadID.x) + 0.5) / float(Width);
	uv.y = (float(Params.DispatchThreadID.y) + 0.5) / float(Height);

	float2 v = BRDF(uv.x, 1.0 - uv.y);

	RWTexture2D<float4> Output		   = HLSL_RWTEXTURE2D(OutputIndex);
	Output[Params.DispatchThreadID.xy] = float4(v, 0.0f, 1.0f);
}
