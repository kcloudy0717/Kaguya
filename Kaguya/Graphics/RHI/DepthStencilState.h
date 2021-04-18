#pragma once
#include <d3d12.h>
#include "AbstractionLayer.h"

class DepthStencilState
{
public:
	enum class Face
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class StencilOperation
	{
		Keep,				// Keep the stencil value
		Zero,				// Set the stencil value to zero
		Replace,			// Replace the stencil value with the reference value
		IncreaseSaturate,   // Increase the stencil value by one, clamp if necessary
		DecreaseSaturate,   // Decrease the stencil value by one, clamp if necessary
		Invert,				// Invert the stencil data (bitwise not)
		Increase,			// Increase the stencil value by one, wrap if necessary
		Decrease			// Decrease the stencil value by one, wrap if necessary
	};

	struct Stencil
	{
		StencilOperation StencilFailOp = StencilOperation::Keep;		// Stencil operation in case stencil test fails
		StencilOperation StencilDepthFailOp = StencilOperation::Keep;	// Stencil operation in case stencil test passes but depth test fails
		StencilOperation StencilPassOp = StencilOperation::Keep;		// Stencil operation in case stencil and depth tests pass
		ComparisonFunc StencilFunc = ComparisonFunc::Always;			// Stencil comparison function

		operator D3D12_DEPTH_STENCILOP_DESC() const noexcept;
	};

	DepthStencilState();

	void SetDepthEnable(bool DepthEnable);

	void SetDepthWrite(bool DepthWrite);

	void SetDepthFunc(ComparisonFunc DepthFunc);

	void SetStencilEnable(bool StencilEnable);

	void SetStencilReadMask(UINT8 StencilReadMask);

	void SetStencilWriteMask(UINT8 StencilWriteMask);

	void SetStencilOp
	(
		Face Face,
		StencilOperation StencilFailOp,
		StencilOperation StencilDepthFailOp,
		StencilOperation StencilPassOp
	);

	void SetStencilFunc(Face Face, ComparisonFunc StencilFunc);

	operator D3D12_DEPTH_STENCIL_DESC() const noexcept;
private:
	bool			m_DepthEnable;
	bool			m_DepthWrite;
	ComparisonFunc	m_DepthFunc;
	bool			m_StencilEnable;
	UINT8			m_StencilReadMask;
	UINT8			m_StencilWriteMask;
	Stencil			m_FrontFace;
	Stencil			m_BackFace;
};

D3D12_STENCIL_OP GetD3D12StencilOp(DepthStencilState::StencilOperation Op);