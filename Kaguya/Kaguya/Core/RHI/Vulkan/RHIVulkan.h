#pragma once
#include <Core/RHI/RHICommon.h>

VkFormat ToVkFormat(ERHIFormat RHIFormat);

constexpr VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology PrimitiveTopology)
{
	// clang-format off
	switch (PrimitiveTopology)
	{
	case PrimitiveTopology::Undefined:	return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	case PrimitiveTopology::Point:		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveTopology::Line:		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveTopology::Triangle:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case PrimitiveTopology::Patch:		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:							return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkCompareOp ToVkCompareOp(ComparisonFunc Func)
{
	// clang-format off
	switch (Func)
	{
	case ComparisonFunc::Never:			return VK_COMPARE_OP_NEVER;
	case ComparisonFunc::Less:			return VK_COMPARE_OP_LESS;
	case ComparisonFunc::Equal:			return VK_COMPARE_OP_EQUAL;
	case ComparisonFunc::LessEqual:		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case ComparisonFunc::Greater:		return VK_COMPARE_OP_GREATER;
	case ComparisonFunc::NotEqual:		return VK_COMPARE_OP_NOT_EQUAL;
	case ComparisonFunc::GreaterEqual:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case ComparisonFunc::Always:		return VK_COMPARE_OP_ALWAYS;
	default:							return VK_COMPARE_OP_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkDescriptorType ToVkDescriptorType(EDescriptorType DescriptorType)
{
	// clang-format off
	switch (DescriptorType)
	{
	case EDescriptorType::ConstantBuffer:	return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case EDescriptorType::Texture:			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EDescriptorType::RWTexture:		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case EDescriptorType::Sampler:			return VK_DESCRIPTOR_TYPE_SAMPLER;
	default:								return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkImageType ToVkImageType(ERHITextureType RHITextureType)
{
	switch (RHITextureType)
	{
	case ERHITextureType::Texture2D:
	case ERHITextureType::Texture2DArray:
	case ERHITextureType::TextureCube:
		return VK_IMAGE_TYPE_2D;

	case ERHITextureType::Texture3D:
		return VK_IMAGE_TYPE_3D;

	case ERHITextureType::Unknown:
	default:
		return VK_IMAGE_TYPE_MAX_ENUM;
	}
}

constexpr VkBlendFactor ToVkBlendFactor(BlendState::Factor Factor)
{
	// clang-format off
	switch (Factor)
	{
	case BlendState::Factor::Zero:					return VK_BLEND_FACTOR_ZERO;
	case BlendState::Factor::One:					return VK_BLEND_FACTOR_ONE;
	case BlendState::Factor::SrcColor:				return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendState::Factor::OneMinusSrcColor:		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendState::Factor::DstColor:				return VK_BLEND_FACTOR_DST_COLOR;
	case BlendState::Factor::OneMinusDstColor:		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendState::Factor::SrcAlpha:				return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendState::Factor::OneMinusSrcAlpha:		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendState::Factor::DstAlpha:				return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendState::Factor::OneMinusDstAlpha:		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case BlendState::Factor::BlendFactor:			return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendState::Factor::OneMinusBlendFactor:	return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendState::Factor::SrcAlphaSaturate:		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case BlendState::Factor::Src1Color:				return VK_BLEND_FACTOR_SRC1_COLOR;
	case BlendState::Factor::OneMinusSrc1Color:		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case BlendState::Factor::Src1Alpha:				return VK_BLEND_FACTOR_SRC1_ALPHA;
	case BlendState::Factor::OneMinusSrc1Alpha:		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	default:										return VK_BLEND_FACTOR_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkBlendOp ToVkBlendOp(BlendState::BlendOp BlendOp)
{
	// clang-format off
	switch (BlendOp)
	{
		case BlendState::BlendOp::Add:				return VK_BLEND_OP_ADD;
		case BlendState::BlendOp::Subtract:			return VK_BLEND_OP_SUBTRACT;
		case BlendState::BlendOp::ReverseSubtract:	return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendState::BlendOp::Min:				return VK_BLEND_OP_MIN;
		case BlendState::BlendOp::Max:				return VK_BLEND_OP_MAX;
		default:									return VK_BLEND_OP_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkPolygonMode ToVkFillMode(RasterizerState::EFillMode FillMode)
{
	// clang-format off
	switch (FillMode)
	{
	case RasterizerState::EFillMode::Wireframe: return VK_POLYGON_MODE_LINE;
	case RasterizerState::EFillMode::Solid:		return VK_POLYGON_MODE_FILL;
	default:									return VK_POLYGON_MODE_MAX_ENUM;
	}
	// clang-format on
}

constexpr VkCullModeFlags ToVkCullMode(RasterizerState::ECullMode CullMode)
{
	// clang-format off
	switch (CullMode)
	{
	case RasterizerState::ECullMode::None:	return VK_CULL_MODE_NONE;
	case RasterizerState::ECullMode::Front: return VK_CULL_MODE_FRONT_BIT;
	case RasterizerState::ECullMode::Back:	return VK_CULL_MODE_BACK_BIT;
	default:								return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	}
	// clang-format on
}
