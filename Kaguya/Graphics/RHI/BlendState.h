#pragma once
#include <d3d12.h>
#include "AbstractionLayer.h"

class BlendState
{
public:
	enum Operation
	{
		None,
		Blend,
		Logic
	};

	/* Defines how to combine the blend inputs */
	enum class BlendOp
	{
		Add,				// Add src1 and src2
		Subtract,			// Subtract src1 from src2
		ReverseSubstract,	// Subtract src2 from src1
		Min,				// Find the minimum between the sources (per-channel)
		Max					// Find the maximum between the sources (per-channel)
	};

	enum class LogicOp
	{
		Clear,			// Clears the render target (0)
		Set,			// Sets the render target (1)
		Copy,			// Copys the render target (s source from Pixel Shader output)
		CopyInverted,	// Performs an inverted-copy of the render target (~s)
		NoOperation,	// No operation is performed on the render target (d destination in the Render Target View)
		Invert,			// Inverts the render target (~d)
		And,			// Performs a logical AND operation on the render target (s & d)
		Nand,			// Performs a logical NAND operation on the render target (~(s & d))
		Or,				// Performs a logical OR operation on the render target (s | d)
		Nor,			// Performs a logical NOR operation on the render target ~(s | d)
		Xor,			// Performs a logical XOR operation on the render target (s ^ d)
		Equivalence,	// Performs a logical equal operation on the render target (~(s ^ d))
		AndReverse,		// Performs a logical AND and reverse operation on the render target (s & ~d)
		AndInverted,	// Performs a logical AND and invert operation on the render target (~s & d)
		OrReverse,		// Performs a logical OR and reverse operation on the render target (s	| ~d)
		OrInverted		// Performs a logical OR and invert operation on the render target (~s	| d)
	};

	/* Defines how to modulate the pixel-shader and render-target pixel values */
	enum class Factor
	{
		Zero,					// (0, 0, 0, 0)
		One,					// (1, 1, 1, 1)
		SrcColor,				// The pixel-shader output color
		OneMinusSrcColor,		// One minus the pixel-shader output color
		DstColor,				// The render-target color
		OneMinusDstColor,		// One minus the render-target color
		SrcAlpha,				// The pixel-shader output alpha value
		OneMinusSrcAlpha,		// One minus the pixel-shader output alpha value
		DstAlpha,				// The render-target alpha value
		OneMinusDstAlpha,		// One minus the render-target alpha value
		BlendFactor,			// Constant color, set using Desc#SetBlendFactor()
		OneMinusBlendFactor,	// One minus constant color, set using Desc#SetBlendFactor()
		SrcAlphaSaturate,		// (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
		Src1Color,				// Fragment-shader output color 1
		OneMinusSrc1Color,		// One minus pixel-shader output color 1
		Src1Alpha,				// Fragment-shader output alpha 1
		OneMinusSrc1Alpha		// One minus pixel-shader output alpha 1
	};

	struct RenderTarget
	{
		Operation	Operation		= Operation::None;
		Factor		SrcBlendRGB		= Factor::One;
		Factor		DstBlendRGB		= Factor::Zero;
		BlendOp		BlendOpRGB		= BlendOp::Add;
		Factor		SrcBlendAlpha	= Factor::One;
		Factor		DstBlendAlpha	= Factor::Zero;
		BlendOp		BlendOpAlpha	= BlendOp::Add;
		LogicOp		LogicOpRGB		= LogicOp::NoOperation;

		struct WriteMask
		{
			bool WriteRed	= true;
			bool WriteGreen = true;
			bool WriteBlue	= true;
			bool WriteAlpha = true;
		} WriteMask;

		operator D3D12_RENDER_TARGET_BLEND_DESC() const noexcept;
	};

	BlendState();

	void SetAlphaToCoverageEnable(bool AlphaToCoverageEnable);
	void SetIndependentBlendEnable(bool IndependentBlendEnable);

	void AddRenderTargetForBlendOp(Factor SrcBlendRGB, Factor DstBlendRGB, BlendOp BlendOpRGB,
		Factor SrcBlendAlpha, Factor DstBlendAlpha, BlendOp BlendOpAlpha);
	void AddRenderTargetForLogicOp(LogicOp LogicOpRGB);

	operator D3D12_BLEND_DESC() const noexcept;
private:
	bool			m_AlphaToCoverageEnable;
	bool			m_IndependentBlendEnable;
	UINT			m_NumRenderTargets;
	RenderTarget	m_RenderTargets[8];
};

D3D12_BLEND_OP GetD3D12BlendOp(BlendState::BlendOp Op);
D3D12_LOGIC_OP GetD3D12LogicOp(BlendState::LogicOp Op);
D3D12_BLEND GetD3D12Blend(BlendState::Factor Factor);