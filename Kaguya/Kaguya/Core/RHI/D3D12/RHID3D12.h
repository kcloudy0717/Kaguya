#pragma once
#include <Core/RHI/RHICore.h>

DXGI_FORMAT ToDxgiFormat(ERHIFormat RHIFormat);

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToD3D12BeginAccessType(ELoadOp Op)
{
	// clang-format off
	switch (Op)
	{
	case ELoadOp::Load:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	case ELoadOp::Clear:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
	case ELoadOp::Noop:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	default:				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
	// clang-format on
}

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToD3D12EndAccessType(EStoreOp Op)
{
	// clang-format off
	switch (Op)
	{
	case EStoreOp::Store:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	case EStoreOp::Noop:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	default:				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
	// clang-format on
}
