#pragma once
#include <Core/RHI/RHICommon.h>

VkFormat ToVkFormat(ERHIFormat RHIFormat);

constexpr VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology PrimitiveTopology)
{
	switch (PrimitiveTopology)
	{
	case PrimitiveTopology::Undefined:
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	case PrimitiveTopology::Point:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveTopology::Line:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveTopology::Triangle:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case PrimitiveTopology::Patch:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}

constexpr VkCompareOp ToVkCompareOp(ComparisonFunc Func)
{
	switch (Func)
	{
	case ComparisonFunc::Never:
		return VK_COMPARE_OP_NEVER;
	case ComparisonFunc::Less:
		return VK_COMPARE_OP_LESS;
	case ComparisonFunc::Equal:
		return VK_COMPARE_OP_EQUAL;
	case ComparisonFunc::LessEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case ComparisonFunc::Greater:
		return VK_COMPARE_OP_GREATER;
	case ComparisonFunc::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case ComparisonFunc::GreaterEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case ComparisonFunc::Always:
		return VK_COMPARE_OP_ALWAYS;
	default:
		return VK_COMPARE_OP_MAX_ENUM;
	}
}

constexpr VkDescriptorType ToVkDescriptorType(EDescriptorType DescriptorType)
{
	switch (DescriptorType)
	{
	case EDescriptorType::ConstantBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	case EDescriptorType::Texture:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

	case EDescriptorType::RWTexture:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	case EDescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;

	default:
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
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

constexpr VkPolygonMode ToVkFillMode(RasterizerState::EFillMode FillMode)
{
	switch (FillMode)
	{
	case RasterizerState::EFillMode::Wireframe:
		return VK_POLYGON_MODE_LINE;
	case RasterizerState::EFillMode::Solid:
		return VK_POLYGON_MODE_FILL;
	default:
		return VK_POLYGON_MODE_MAX_ENUM;
	}
}

constexpr VkCullModeFlags ToVkCullMode(RasterizerState::ECullMode CullMode)
{
	switch (CullMode)
	{
	case RasterizerState::ECullMode::None:
		return VK_CULL_MODE_NONE;
	case RasterizerState::ECullMode::Front:
		return VK_CULL_MODE_FRONT_BIT;
	case RasterizerState::ECullMode::Back:
		return VK_CULL_MODE_BACK_BIT;
	default:
		return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	}
}
