#include "VulkanResource.h"

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
	}
}

VulkanTexture::VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc)
	: IRHITexture(Parent)
	, AllocationDesc(AllocationDesc)
	, Desc(Desc)
{
	VERIFY_VULKAN_API(vmaCreateImage(Parent->GetVkAllocator(), &Desc, &AllocationDesc, &Texture, &Allocation, nullptr));

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
		for (auto View : ImageViewTable | std::views::values)
		{
			vkDestroyImageView(Parent->As<VulkanDevice>()->GetVkDevice(), View, nullptr);
		}
		vkDestroyImageView(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(ImageView, {}), nullptr);
		vmaDestroyImage(
			Parent->As<VulkanDevice>()->GetVkAllocator(),
			std::exchange(Texture, {}),
			std::exchange(Allocation, {}));
	}
}
