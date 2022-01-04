#pragma once

/*
	Defines bindless descriptor table, resources are grouped together based on their usage [Read/Write] within
	their respected descriptor tables
*/

// ShaderResource
ByteAddressBuffer g_ByteAddressBufferTable[] : register(t0, space100);
Texture2D		  g_Texture2DTable[] : register(t0, space101);
Texture2DArray	  g_Texture2DArrayTable[] : register(t0, space102);
TextureCube		  g_TextureCubeTable[] : register(t0, space103);

// UnorderedAccess
RWTexture2D<float4> g_RWTexture2DTable[] : register(u0, space100);

// Sampler
SamplerState g_SamplerTable[] : register(s0, space100);

// Defines global static samplers
SamplerState g_SamplerPointWrap : register(s0, space101);
SamplerState g_SamplerPointClamp : register(s1, space101);

SamplerState g_SamplerLinearWrap : register(s2, space101);
SamplerState g_SamplerLinearClamp : register(s3, space101);
SamplerState g_SamplerLinearBorder : register(s4, space101);

SamplerState g_SamplerAnisotropicWrap : register(s5, space101);
SamplerState g_SamplerAnisotropicClamp : register(s6, space101);
