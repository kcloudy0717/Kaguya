#include "Shader.hlsli"
#include "HlslDynamicResource.hlsli"

#include "HLSLCommon.hlsli"

cbuffer Parameters : register(b0)
{
	// Constants
	Camera Camera;
	uint2  Viewport;
	uint   NumLights;

	// Texture indices
	uint AlbedoTextureIndex;
	uint NormalTextureIndex;
	uint MaterialTextureIndex;
	uint DepthTextureIndex;

	uint OutputTextureIndex;
};

StructuredBuffer<Light> g_Lights : register(t0, space0);

[numthreads(8, 8, 1)] void CSMain(CSParams Params)
{
	// Discard oob pixels
	if (any(Params.DispatchThreadID.xy >= Viewport))
	{
		return;
	}

	Texture2D Depth		 = HLSL_TEXTURE2D(DepthTextureIndex);
	float	  DepthValue = Depth[Params.DispatchThreadID.xy].r;
	if (DepthValue >= 1.0f)
	{
		return;
	}

	Texture2D AlbedoTexture	  = HLSL_TEXTURE2D(AlbedoTextureIndex);
	Texture2D MaterialTexture = HLSL_TEXTURE2D(MaterialTextureIndex);
	Texture2D NormalTexture	  = HLSL_TEXTURE2D(NormalTextureIndex);

	float3 Albedo		 = AlbedoTexture[Params.DispatchThreadID.xy].rgb;
	float  Roughness	 = MaterialTexture[Params.DispatchThreadID.xy].r;
	float  Metalness	 = MaterialTexture[Params.DispatchThreadID.xy].g;
	float3 WorldPosition = NDCDepthToWorldPosition(DepthValue, (float2(Params.DispatchThreadID.xy) + 0.5f) / float2(Viewport), Camera);
	float3 WorldNormal	 = NormalTexture[Params.DispatchThreadID.xy].xyz;
	float3 ViewDirection = normalize(Camera.Position.xyz - WorldPosition);

	float3 color = float3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < NumLights; i++)
	{
		Light light = g_Lights[i];

		if (light.Type == LightType_Point)
		{
			float3 L = light.Position - WorldPosition;
			float  d = length(L);
			if (d > light.Radius)
			{
				continue;
			}

			L /= d;

			float attenuation = saturate(1.0f - d * d / (light.Radius * light.Radius));
			attenuation *= attenuation;

			float3 radiance = light.I * light.Intensity * attenuation;

			float3 HalfVector = normalize(ViewDirection + L);
			color += BRDF(WorldNormal, ViewDirection, HalfVector, L, Albedo, Roughness, Metalness, radiance);
		}
	}

	RWTexture2D<float4> Output		   = HLSL_RWTEXTURE2D(OutputTextureIndex);
	Output[Params.DispatchThreadID.xy] = float4(color, 1.0f);
}
