#ifndef __SHARED_DEFINES_HLSLI__
#define __SHARED_DEFINES_HLSLI__
#ifdef __cplusplus
#include <DirectXMath.h>

#define row_major
#define matrix DirectX::XMFLOAT4X4
#define float4x4 matrix

#define float4 DirectX::XMFLOAT4
#define float3 DirectX::XMFLOAT3
#define float2 DirectX::XMFLOAT2

#define int4 DirectX::XMINT4
#define int3 DirectX::XMINT3
#define int2 DirectX::XMINT2
#endif
#endif