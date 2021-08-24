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
	VERIFY_VULKAN_API(vmaMapMemory(GetParentDevice()->GetVkAllocator(), Allocation, &CPUVirtualAddress));
	{
		if (CPUVirtualAddress)
		{
			Function(CPUVirtualAddress);
		}
	}
	vmaUnmapMemory(GetParentDevice()->GetVkAllocator(), Allocation);
}

VulkanBuffer::~VulkanBuffer()
{
	if (Parent && Allocation && Buffer)
	{
		vmaDestroyBuffer(GetParentDevice()->GetVkAllocator(), Buffer, Allocation);
	}
}

auto VulkanBuffer::GetParentDevice() noexcept -> VulkanDevice*
{
	return static_cast<VulkanDevice*>(Parent);
}

VulkanTexture::VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc)
	: IRHITexture(Parent)
	, AllocationDesc(AllocationDesc)
	, Desc(Desc)
{
	VERIFY_VULKAN_API(vmaCreateImage(Parent->GetVkAllocator(), &Desc, &AllocationDesc, &Texture, &Allocation, nullptr));
}

VulkanTexture::~VulkanTexture()
{
	if (Parent && Allocation && Texture)
	{
		vmaDestroyImage(GetParentDevice()->GetVkAllocator(), Texture, Allocation);
	}
}

auto VulkanTexture::GetParentDevice() noexcept -> VulkanDevice*
{
	return static_cast<VulkanDevice*>(Parent);
}
