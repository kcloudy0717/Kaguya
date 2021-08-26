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
	VERIFY_VULKAN_API(vmaMapMemory(GetVulkanDevice()->GetVkAllocator(), Allocation, &CPUVirtualAddress));
	{
		if (CPUVirtualAddress)
		{
			Function(CPUVirtualAddress);
		}
	}
	vmaUnmapMemory(GetVulkanDevice()->GetVkAllocator(), Allocation);
}

VulkanBuffer::~VulkanBuffer()
{
	if (Parent && Allocation && Buffer)
	{
		vmaDestroyBuffer(GetVulkanDevice()->GetVkAllocator(), std::exchange(Buffer, {}), std::exchange(Allocation, {}));
		GetVulkanDevice()->GetResourcePool<VulkanBuffer>().Destruct(this);
	}
}

auto VulkanBuffer::GetVulkanDevice() const noexcept -> VulkanDevice*
{
	return static_cast<VulkanDevice*>(Parent);
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
	if (Parent && Allocation && Texture)
	{
		vkDestroyImageView(GetVulkanDevice()->GetVkDevice(), std::exchange(ImageView, {}), nullptr);
		vmaDestroyImage(GetVulkanDevice()->GetVkAllocator(), std::exchange(Texture, {}), std::exchange(Allocation, {}));
		GetVulkanDevice()->GetResourcePool<VulkanTexture>().Destruct(this);
	}
}

auto VulkanTexture::GetVulkanDevice() const noexcept -> VulkanDevice*
{
	return static_cast<VulkanDevice*>(Parent);
}
