#pragma once
#include "RHI.h"

struct HlslByteAddressBuffer
{
	HlslByteAddressBuffer() noexcept = default;
	HlslByteAddressBuffer(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle;
};

struct HlslTexture2D
{
	HlslTexture2D() noexcept = default;
	HlslTexture2D(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle;
};

struct HlslTexture2DArray
{
	HlslTexture2DArray() noexcept = default;
	HlslTexture2DArray(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle;
};

struct HlslTextureCube
{
	HlslTextureCube() noexcept = default;
	HlslTextureCube(const RHI::D3D12ShaderResourceView* ShaderResourceView)
		: Handle(ShaderResourceView->GetIndex())
	{
	}

	u32 Handle;
};

struct HlslRWTexture2D
{
	HlslRWTexture2D() noexcept = default;
	HlslRWTexture2D(const RHI::D3D12UnorderedAccessView* UnorderedAccessView)
		: Handle(UnorderedAccessView->GetIndex())
	{
	}

	u32 Handle;
};

struct HlslSamplerState
{
	u32 Handle;
};
