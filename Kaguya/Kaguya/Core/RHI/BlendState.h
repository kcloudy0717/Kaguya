#pragma once

class BlendState
{
public:
	[[nodiscard]] explicit operator D3D12_BLEND_DESC() const noexcept;

	/// <summary>
	/// Defines how to combine the blend inputs
	/// </summary>
	enum class BlendOp
	{
		Add,			 // Add src1 and src2
		Subtract,		 // Subtract src1 from src2
		ReverseSubtract, // Subtract src2 from src1
		Min,			 // Find the minimum between the sources (per-channel)
		Max				 // Find the maximum between the sources (per-channel)
	};

	/// <summary>
	/// Defines how to modulate the pixel-shader and render-target pixel values
	/// </summary>
	enum class Factor
	{
		Zero,				 // (0, 0, 0, 0)
		One,				 // (1, 1, 1, 1)
		SrcColor,			 // The pixel-shader output color
		OneMinusSrcColor,	 // One minus the pixel-shader output color
		DstColor,			 // The render-target color
		OneMinusDstColor,	 // One minus the render-target color
		SrcAlpha,			 // The pixel-shader output alpha value
		OneMinusSrcAlpha,	 // One minus the pixel-shader output alpha value
		DstAlpha,			 // The render-target alpha value
		OneMinusDstAlpha,	 // One minus the render-target alpha value
		BlendFactor,		 // Constant color, set using Desc#SetBlendFactor()
		OneMinusBlendFactor, // One minus constant color, set using Desc#SetBlendFactor()
		SrcAlphaSaturate,	 // (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
		Src1Color,			 // Fragment-shader output color 1
		OneMinusSrc1Color,	 // One minus pixel-shader output color 1
		Src1Alpha,			 // Fragment-shader output alpha 1
		OneMinusSrc1Alpha	 // One minus pixel-shader output alpha 1
	};

	struct RenderTarget
	{
		operator D3D12_RENDER_TARGET_BLEND_DESC() const noexcept;

		bool	BlendEnable	  = false;
		Factor	SrcBlendRgb	  = Factor::One;
		Factor	DstBlendRgb	  = Factor::Zero;
		BlendOp BlendOpRgb	  = BlendOp::Add;
		Factor	SrcBlendAlpha = Factor::One;
		Factor	DstBlendAlpha = Factor::Zero;
		BlendOp BlendOpAlpha  = BlendOp::Add;

		struct WriteMask
		{
			bool R = true;
			bool G = true;
			bool B = true;
			bool A = true;
		} WriteMask;
	};

	BlendState();

	void SetAlphaToCoverageEnable(bool AlphaToCoverageEnable);
	void SetIndependentBlendEnable(bool IndependentBlendEnable);

	void SetRenderTargetBlendDesc(
		UINT	Index,
		Factor	SrcBlendRgb,
		Factor	DstBlendRgb,
		BlendOp BlendOpRgb,
		Factor	SrcBlendAlpha,
		Factor	DstBlendAlpha,
		BlendOp BlendOpAlpha);

	bool		 AlphaToCoverageEnable;
	bool		 IndependentBlendEnable;
	RenderTarget RenderTargets[8];
};

constexpr D3D12_BLEND_OP ToD3D12BlendOp(BlendState::BlendOp Op)
{
	// clang-format off
	switch (Op)
	{
	case BlendState::BlendOp::Add:				return D3D12_BLEND_OP_ADD;
	case BlendState::BlendOp::Subtract:			return D3D12_BLEND_OP_SUBTRACT;
	case BlendState::BlendOp::ReverseSubtract:	return D3D12_BLEND_OP_REV_SUBTRACT;
	case BlendState::BlendOp::Min:				return D3D12_BLEND_OP_MIN;
	case BlendState::BlendOp::Max:				return D3D12_BLEND_OP_MAX;
	default:									return D3D12_BLEND_OP();
	}
	// clang-format on
}

constexpr D3D12_BLEND ToD3D12Blend(BlendState::Factor Factor)
{
	// clang-format off
	switch (Factor)
	{
	case BlendState::Factor::Zero:					return D3D12_BLEND_ZERO;
	case BlendState::Factor::One:					return D3D12_BLEND_ONE;
	case BlendState::Factor::SrcColor:				return D3D12_BLEND_SRC_COLOR;
	case BlendState::Factor::OneMinusSrcColor:		return D3D12_BLEND_INV_SRC_COLOR;
	case BlendState::Factor::DstColor:				return D3D12_BLEND_DEST_COLOR;
	case BlendState::Factor::OneMinusDstColor:		return D3D12_BLEND_INV_DEST_COLOR;
	case BlendState::Factor::SrcAlpha:				return D3D12_BLEND_SRC_ALPHA;
	case BlendState::Factor::OneMinusSrcAlpha:		return D3D12_BLEND_INV_SRC_ALPHA;
	case BlendState::Factor::DstAlpha:				return D3D12_BLEND_DEST_ALPHA;
	case BlendState::Factor::OneMinusDstAlpha:		return D3D12_BLEND_INV_DEST_ALPHA;
	case BlendState::Factor::BlendFactor:			return D3D12_BLEND_BLEND_FACTOR;
	case BlendState::Factor::OneMinusBlendFactor:	return D3D12_BLEND_INV_BLEND_FACTOR;
	case BlendState::Factor::SrcAlphaSaturate:		return D3D12_BLEND_SRC_ALPHA_SAT;
	case BlendState::Factor::Src1Color:				return D3D12_BLEND_SRC1_COLOR;
	case BlendState::Factor::OneMinusSrc1Color:		return D3D12_BLEND_INV_SRC1_COLOR;
	case BlendState::Factor::Src1Alpha:				return D3D12_BLEND_SRC1_ALPHA;
	case BlendState::Factor::OneMinusSrc1Alpha:		return D3D12_BLEND_INV_SRC1_ALPHA;
	default:										return D3D12_BLEND();
	}
	// clang-format on
}
