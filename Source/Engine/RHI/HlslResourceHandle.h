#pragma once
#include "RHI.h"

#define HLSL_INVALID_RESOURCE_HANDLE (-1)

struct HlslByteAddressBuffer
{
	HlslByteAddressBuffer() noexcept = default;
	HlslByteAddressBuffer(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};

struct HlslTexture2D
{
	HlslTexture2D() noexcept = default;
	HlslTexture2D(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};

struct HlslTexture2DArray
{
	HlslTexture2DArray() noexcept = default;
	HlslTexture2DArray(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};

struct HlslTextureCube
{
	HlslTextureCube() noexcept = default;
	HlslTextureCube(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};

struct HlslRWTexture2D
{
	HlslRWTexture2D() noexcept = default;
	HlslRWTexture2D(const RHI::D3D12UnorderedAccessView* UnorderedAccessView)
		: Handle(UnorderedAccessView->GetIndex())
	{
	}

	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};

struct HlslSamplerState
{
	u32 Handle = HLSL_INVALID_RESOURCE_HANDLE;
};
