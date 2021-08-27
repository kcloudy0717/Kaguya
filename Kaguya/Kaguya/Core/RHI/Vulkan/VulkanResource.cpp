#include "VulkanResource.h"

// infer aspect flags for a given image format
VkImageAspectFlags InferImageAspectFlags(VkFormat Format)
{
	switch (Format) // NOLINT(clang-diagnostic-switch-enum)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
		return VK_IMAGE_ASPECT_DEPTH_BIT;

	case VK_FORMAT_S8_UINT:
		return VK_IMAGE_ASPECT_STENCIL_BIT;

	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VulkanBuffer::VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc)
	: IRHIBuffer(Parent)
	, AllocationDesc(AllocationDesc)
	, Desc(Desc)
{
	VERIFY_VULKAN_API(vmaCreateBuffer(Parent->GetVkAllocator(), &Desc, &AllocationDesc, &Buffer, &Allocation, nullptr));
}

void VulkanBuffer::Upload(std::function<void(void* CPUVirtualAddress)> Function)
{
	void* CPUVirtualAddress = nullptr;
	VERIFY_VULKAN_API(vmaMapMemory(Parent->As<VulkanDevice>()->GetVkAllocator(), Allocation, &CPUVirtualAddress));
	{
		if (CPUVirtualAddress)
		{
			Function(CPUVirtualAddress);
		}
	}
	vmaUnmapMemory(Parent->As<VulkanDevice>()->GetVkAllocator(), Allocation);
}

VulkanBuffer::~VulkanBuffer()
{
	if (Parent && Allocation && Buffer)
	{
		vmaDestroyBuffer(
			Parent->As<VulkanDevice>()->GetVkAllocator(),
			std::exchange(Buffer, {}),
			std::exchange(Allocation, {}));
		Parent->As<VulkanDevice>()->GetResourcePool<VulkanBuffer>().Destruct(this);
	}
}

VulkanTexture::VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc)
	: IRHITexture(Parent)
	, AllocationDesc(AllocationDesc)
	, Desc(Desc)
{
	VERIFY_VULKAN_API(vmaCreateImage(Parent->GetVkAllocator(), &Desc, &AllocationDesc, &Texture, &Allocation, nullptr));

	// build a image-view for the depth image to use for rendering
	auto ImageViewCreateInfo							= VkStruct<VkImageViewCreateInfo>();
	ImageViewCreateInfo.viewType						= VK_IMAGE_VIEW_TYPE_2D;
	ImageViewCreateInfo.image							= Texture;
	ImageViewCreateInfo.format							= Desc.format;
	ImageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	ImageViewCreateInfo.subresourceRange.levelCount		= 1;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.layerCount		= 1;
	ImageViewCreateInfo.subresourceRange.aspectMask		= InferImageAspectFlags(Desc.format);
	VERIFY_VULKAN_API(vkCreateImageView(Parent->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

VulkanTexture::~VulkanTexture()
{
	if (Parent && Allocation && Texture && ImageView)
	{
		vkDestroyImageView(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(ImageView, {}), nullptr);
		vmaDestroyImage(
			Parent->As<VulkanDevice>()->GetVkAllocator(),
			std::exchange(Texture, {}),
			std::exchange(Allocation, {}));
		Parent->As<VulkanDevice>()->GetResourcePool<VulkanTexture>().Destruct(this);
	}
}
