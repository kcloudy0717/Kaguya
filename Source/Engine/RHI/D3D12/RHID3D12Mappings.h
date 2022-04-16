#pragma once
#include "D3D12Core.h"

namespace RHI
{

	constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE RHITranslateD3D12(RHI_PRIMITIVE_TOPOLOGY Topology)
	{
		// clang-format off
		switch (Topology)
		{
			using enum RHI_PRIMITIVE_TOPOLOGY;
		case Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		case Point:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case Line:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case Triangle:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case Patch:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_COMPARISON_FUNC RHITranslateD3D12(RHI_COMPARISON_FUNC Func)
	{
		// clang-format off
		switch (Func)
		{
			using enum RHI_COMPARISON_FUNC;
		case Never:			return D3D12_COMPARISON_FUNC_NEVER;
		case Less:			return D3D12_COMPARISON_FUNC_LESS;
		case Equal:			return D3D12_COMPARISON_FUNC_EQUAL;
		case LessEqual:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case Greater:		return D3D12_COMPARISON_FUNC_GREATER;
		case NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case GreaterEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case Always:		return D3D12_COMPARISON_FUNC_ALWAYS;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_BLEND_OP RHITranslateD3D12(RHI_BLEND_OP Op)
	{
		// clang-format off
		switch (Op)
		{
			using enum RHI_BLEND_OP;
		case Add:				return D3D12_BLEND_OP_ADD;
		case Subtract:			return D3D12_BLEND_OP_SUBTRACT;
		case ReverseSubtract:	return D3D12_BLEND_OP_REV_SUBTRACT;
		case Min:				return D3D12_BLEND_OP_MIN;
		case Max:				return D3D12_BLEND_OP_MAX;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_BLEND RHITranslateD3D12(RHI_FACTOR Factor)
	{
		// clang-format off
		switch (Factor)
		{
			using enum RHI_FACTOR;
		case Zero:					return D3D12_BLEND_ZERO;
		case One:					return D3D12_BLEND_ONE;
		case SrcColor:				return D3D12_BLEND_SRC_COLOR;
		case OneMinusSrcColor:		return D3D12_BLEND_INV_SRC_COLOR;
		case DstColor:				return D3D12_BLEND_DEST_COLOR;
		case OneMinusDstColor:		return D3D12_BLEND_INV_DEST_COLOR;
		case SrcAlpha:				return D3D12_BLEND_SRC_ALPHA;
		case OneMinusSrcAlpha:		return D3D12_BLEND_INV_SRC_ALPHA;
		case DstAlpha:				return D3D12_BLEND_DEST_ALPHA;
		case OneMinusDstAlpha:		return D3D12_BLEND_INV_DEST_ALPHA;
		case BlendFactor:			return D3D12_BLEND_BLEND_FACTOR;
		case OneMinusBlendFactor:	return D3D12_BLEND_INV_BLEND_FACTOR;
		case SrcAlphaSaturate:		return D3D12_BLEND_SRC_ALPHA_SAT;
		case Src1Color:				return D3D12_BLEND_SRC1_COLOR;
		case OneMinusSrc1Color:		return D3D12_BLEND_INV_SRC1_COLOR;
		case Src1Alpha:				return D3D12_BLEND_SRC1_ALPHA;
		case OneMinusSrc1Alpha:		return D3D12_BLEND_INV_SRC1_ALPHA;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_FILL_MODE RHITranslateD3D12(RHI_FILL_MODE FillMode)
	{
		// clang-format off
		switch (FillMode)
		{
			using enum RHI_FILL_MODE;
		case Wireframe: return D3D12_FILL_MODE_WIREFRAME;
		case Solid:		return D3D12_FILL_MODE_SOLID;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_CULL_MODE RHITranslateD3D12(RHI_CULL_MODE CullMode)
	{
		// clang-format off
		switch (CullMode)
		{
			using enum RHI_CULL_MODE;
		case None:	return D3D12_CULL_MODE_NONE;
		case Front: return D3D12_CULL_MODE_FRONT;
		case Back:	return D3D12_CULL_MODE_BACK;
		}
		// clang-format on
		return {};
	}

	constexpr D3D12_STENCIL_OP RHITranslateD3D12(RHI_STENCIL_OP Op)
	{
		// clang-format off
		switch (Op)
		{
			using enum RHI_STENCIL_OP;
		case Keep:				return D3D12_STENCIL_OP_KEEP;
		case Zero:				return D3D12_STENCIL_OP_ZERO;
		case Replace:			return D3D12_STENCIL_OP_REPLACE;
		case IncreaseSaturate:	return D3D12_STENCIL_OP_INCR_SAT;
		case DecreaseSaturate:	return D3D12_STENCIL_OP_DECR_SAT;
		case Invert:			return D3D12_STENCIL_OP_INVERT;
		case Increase:			return D3D12_STENCIL_OP_INCR;
		case Decrease:			return D3D12_STENCIL_OP_DECR;
		}
		// clang-format on
		return {};
	}

	inline D3D12_DEPTH_STENCILOP_DESC RHITranslateD3D12(const RHIDepthStencilOpDesc& Desc)
	{
		return { RHITranslateD3D12(Desc.StencilFailOp),
				 RHITranslateD3D12(Desc.StencilDepthFailOp),
				 RHITranslateD3D12(Desc.StencilPassOp),
				 RHITranslateD3D12(Desc.StencilFunc) };
	}

	inline D3D12_BLEND_DESC RHITranslateD3D12(const RHIBlendState& BlendState)
	{
		D3D12_BLEND_DESC Desc		= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		Desc.AlphaToCoverageEnable	= BlendState.AlphaToCoverageEnable;
		Desc.IndependentBlendEnable = BlendState.IndependentBlendEnable;
		std::span D3D12RenderTarget = Desc.RenderTarget;
		size_t	  Index				= 0;
		for (auto& RenderTarget : D3D12RenderTarget)
		{
			auto& RHIRenderTarget = BlendState.RenderTargets[Index++];

			RenderTarget.BlendEnable	= RHIRenderTarget.BlendEnable ? TRUE : FALSE;
			RenderTarget.LogicOpEnable	= FALSE;
			RenderTarget.SrcBlend		= RHITranslateD3D12(RHIRenderTarget.SrcBlendRgb);
			RenderTarget.DestBlend		= RHITranslateD3D12(RHIRenderTarget.DstBlendRgb);
			RenderTarget.BlendOp		= RHITranslateD3D12(RHIRenderTarget.BlendOpRgb);
			RenderTarget.SrcBlendAlpha	= RHITranslateD3D12(RHIRenderTarget.SrcBlendAlpha);
			RenderTarget.DestBlendAlpha = RHITranslateD3D12(RHIRenderTarget.DstBlendAlpha);
			RenderTarget.BlendOpAlpha	= RHITranslateD3D12(RHIRenderTarget.BlendOpAlpha);
			RenderTarget.LogicOp		= D3D12_LOGIC_OP_CLEAR;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.R ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.G ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.B ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.A ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
		}
		return Desc;
	}

	inline D3D12_RASTERIZER_DESC RHITranslateD3D12(const RHIRasterizerState& RasterizerState)
	{
		D3D12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		Desc.FillMode			   = RHITranslateD3D12(RasterizerState.FillMode);
		Desc.CullMode			   = RHITranslateD3D12(RasterizerState.CullMode);
		Desc.FrontCounterClockwise = RasterizerState.FrontCounterClockwise;
		Desc.DepthBias			   = RasterizerState.DepthBias;
		Desc.DepthBiasClamp		   = RasterizerState.DepthBiasClamp;
		Desc.SlopeScaledDepthBias  = RasterizerState.SlopeScaledDepthBias;
		Desc.DepthClipEnable	   = RasterizerState.DepthClipEnable;
		Desc.MultisampleEnable	   = RasterizerState.MultisampleEnable;
		Desc.AntialiasedLineEnable = RasterizerState.AntialiasedLineEnable;
		Desc.ForcedSampleCount	   = RasterizerState.ForcedSampleCount;
		Desc.ConservativeRaster	   = RasterizerState.ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON
																		: D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return Desc;
	}

	inline D3D12_DEPTH_STENCIL_DESC RHITranslateD3D12(const RHIDepthStencilState& DepthStencilState)
	{
		D3D12_DEPTH_STENCIL_DESC Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		Desc.DepthEnable			  = DepthStencilState.DepthEnable;
		Desc.DepthWriteMask			  = DepthStencilState.DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc				  = RHITranslateD3D12(DepthStencilState.DepthFunc);
		Desc.StencilEnable			  = DepthStencilState.StencilEnable;
		Desc.StencilReadMask		  = static_cast<UINT8>(DepthStencilState.StencilReadMask);
		Desc.StencilWriteMask		  = static_cast<UINT8>(DepthStencilState.StencilWriteMask);
		Desc.FrontFace				  = RHITranslateD3D12(DepthStencilState.FrontFace);
		Desc.BackFace				  = RHITranslateD3D12(DepthStencilState.BackFace);
		return Desc;
	}
} // namespace RHI
