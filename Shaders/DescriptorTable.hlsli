#ifndef DESCRIPTOR_TABLE_HLSLI
#define DESCRIPTOR_TABLE_HLSLI

/*
	Defines bindless descriptor table, resources are grouped together based on their usage [Read/Write] within
	their respected descriptor tables
*/

// ShaderResource
Texture2D	   g_Texture2DTable[] : register(t0, space100);
Texture2DArray g_Texture2DArrayTable[] : register(t0, space101);
TextureCube	   g_TextureCubeTable[] : register(t0, space102);

// UnorderedAccess
RWTexture2D<float4> g_RWTexture2DTable[] : register(u0, space100);

// Sampler
SamplerState g_SamplerTable[] : register(s0, space100);

#endif // DESCRIPTOR_TABLE_HLSLI
