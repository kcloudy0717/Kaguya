#include "VulkanResource.h"

VulkanResource::~VulkanResource()
{
}

VulkanBuffer::VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc)
	: VulkanResource(Parent, AllocationDesc)
	, Desc(Desc)
{
	VERIFY_VULKAN_API(vmaCreateBuffer(Parent->GetVkAllocator(), &Desc, &AllocationDesc, &Buffer, &Allocation, nullptr));
}

void VulkanBuffer::Upload(std::function<void(void* CPUVirtualAddress)> Function)
{
	void* CPUVirtualAddress = nullptr;
	VERIFY_VULKAN_API(vmaMapMemory(Parent->GetVkAllocator(), Allocation, &CPUVirtualAddress));
	{
		if (CPUVirtualAddress)
		{
			Function(CPUVirtualAddress);
		}
	}
	vmaUnmapMemory(Parent->GetVkAllocator(), Allocation);
}

VulkanBuffer::~VulkanBuffer()
{
	if (Parent && Allocation && Buffer)
	{
		vmaDestroyBuffer(Parent->GetVkAllocator(), Buffer, Allocation);
	}
}
