#pragma once
#include "RHICore.h"

class DepthStencilState
{
public:
	[[nodiscard]] explicit operator D3D12_DEPTH_STENCIL_DESC() const noexcept;

	enum class EFace
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class EStencilOp
	{
		Keep,			  // Keep the stencil value
		Zero,			  // Set the stencil value to zero
		Replace,		  // Replace the stencil value with the reference value
		IncreaseSaturate, // Increase the stencil value by one, clamp if necessary
		DecreaseSaturate, // Decrease the stencil value by one, clamp if necessary
		Invert,			  // Invert the stencil data (bitwise not)
		Increase,		  // Increase the stencil value by one, wrap if necessary
		Decrease		  // Decrease the stencil value by one, wrap if necessary
	};

	struct Stencil
	{
		[[nodiscard]] operator D3D12_DEPTH_STENCILOP_DESC() const noexcept;

		EStencilOp FailOp	   = EStencilOp::Keep; // Stencil operation in case stencil test fails
		EStencilOp DepthFailOp = EStencilOp::Keep; // Stencil operation in case stencil test passes but depth test fails
		EStencilOp PassOp	   = EStencilOp::Keep; // Stencil operation in case stencil and depth tests pass
		ComparisonFunc Func	   = ComparisonFunc::Always; // Stencil comparison function
	};

	DepthStencilState();

	void SetDepthEnable(bool DepthEnable);

	void SetDepthWrite(bool DepthWrite);

	void SetDepthFunc(ComparisonFunc DepthFunc);

	void SetStencilEnable(bool StencilEnable);

	void SetStencilReadMask(UINT8 StencilReadMask);

	void SetStencilWriteMask(UINT8 StencilWriteMask);

	void SetStencilOp(EFace Face, EStencilOp StencilFailOp, EStencilOp StencilDepthFailOp, EStencilOp StencilPassOp);

	void SetStencilFunc(EFace Face, ComparisonFunc StencilFunc);

	bool		   DepthEnable;
	bool		   DepthWrite;
	ComparisonFunc DepthFunc;
	bool		   StencilEnable;
	UINT8		   StencilReadMask;
	UINT8		   StencilWriteMask;
	Stencil		   FrontFace;
	Stencil		   BackFace;
};

constexpr D3D12_STENCIL_OP ToD3D12StencilOp(DepthStencilState::EStencilOp Op)
{
	// clang-format off
	switch (Op)
	{
	case DepthStencilState::EStencilOp::Keep:				return D3D12_STENCIL_OP_KEEP;
	case DepthStencilState::EStencilOp::Zero:				return D3D12_STENCIL_OP_ZERO;
	case DepthStencilState::EStencilOp::Replace:			return D3D12_STENCIL_OP_REPLACE;
	case DepthStencilState::EStencilOp::IncreaseSaturate:	return D3D12_STENCIL_OP_INCR_SAT;
	case DepthStencilState::EStencilOp::DecreaseSaturate:	return D3D12_STENCIL_OP_DECR_SAT;
	case DepthStencilState::EStencilOp::Invert:				return D3D12_STENCIL_OP_INVERT;
	case DepthStencilState::EStencilOp::Increase:			return D3D12_STENCIL_OP_INCR;
	case DepthStencilState::EStencilOp::Decrease:			return D3D12_STENCIL_OP_DECR;
	}
	return D3D12_STENCIL_OP();
	// clang-format on
}
