#pragma once
#include "RHICore.h"

class DepthStencilState
{
public:
	[[nodiscard]] explicit operator D3D12_DEPTH_STENCIL_DESC() const noexcept;

	enum class FACE
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class STENCIL_OP
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

		STENCIL_OP FailOp	   = STENCIL_OP::Keep; // Stencil operation in case stencil test fails
		STENCIL_OP DepthFailOp = STENCIL_OP::Keep; // Stencil operation in case stencil test passes but depth test fails
		STENCIL_OP PassOp	   = STENCIL_OP::Keep; // Stencil operation in case stencil and depth tests pass
		RHI_COMPARISON_FUNC Func	   = RHI_COMPARISON_FUNC::Always; // Stencil comparison function
	};

	DepthStencilState();

	void SetDepthEnable(bool DepthEnable);

	void SetDepthWrite(bool DepthWrite);

	void SetDepthFunc(RHI_COMPARISON_FUNC DepthFunc);

	void SetStencilEnable(bool StencilEnable);

	void SetStencilReadMask(UINT8 StencilReadMask);

	void SetStencilWriteMask(UINT8 StencilWriteMask);

	void SetStencilOp(FACE Face, STENCIL_OP StencilFailOp, STENCIL_OP StencilDepthFailOp, STENCIL_OP StencilPassOp);

	void SetStencilFunc(FACE Face, RHI_COMPARISON_FUNC StencilFunc);

	bool		   DepthEnable;
	bool		   DepthWrite;
	RHI_COMPARISON_FUNC DepthFunc;
	bool		   StencilEnable;
	UINT8		   StencilReadMask;
	UINT8		   StencilWriteMask;
	Stencil		   FrontFace;
	Stencil		   BackFace;
};

constexpr D3D12_STENCIL_OP ToD3D12StencilOp(DepthStencilState::STENCIL_OP Op)
{
	// clang-format off
	switch (Op)
	{
	case DepthStencilState::STENCIL_OP::Keep:				return D3D12_STENCIL_OP_KEEP;
	case DepthStencilState::STENCIL_OP::Zero:				return D3D12_STENCIL_OP_ZERO;
	case DepthStencilState::STENCIL_OP::Replace:			return D3D12_STENCIL_OP_REPLACE;
	case DepthStencilState::STENCIL_OP::IncreaseSaturate:	return D3D12_STENCIL_OP_INCR_SAT;
	case DepthStencilState::STENCIL_OP::DecreaseSaturate:	return D3D12_STENCIL_OP_DECR_SAT;
	case DepthStencilState::STENCIL_OP::Invert:				return D3D12_STENCIL_OP_INVERT;
	case DepthStencilState::STENCIL_OP::Increase:			return D3D12_STENCIL_OP_INCR;
	case DepthStencilState::STENCIL_OP::Decrease:			return D3D12_STENCIL_OP_DECR;
	}
	return D3D12_STENCIL_OP();
	// clang-format on
}
