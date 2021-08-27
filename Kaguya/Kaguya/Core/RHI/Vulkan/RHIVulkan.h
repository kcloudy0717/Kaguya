#pragma once
#include <Core/RHI/RHICommon.h>

VkFormat ToVkFormat(ERHIFormat RHIFormat);

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
