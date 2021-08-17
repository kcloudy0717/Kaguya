#pragma once
#include "VulkanCommon.h"

class VulkanResource : public VulkanDeviceChild
{
public:
	VulkanResource() noexcept = default;
	VulkanResource(VulkanDevice* Parent, VmaAllocationCreateInfo AllocationDesc)
		: VulkanDeviceChild(Parent)
		, AllocationDesc(AllocationDesc)
	{
	}
	~VulkanResource();

	VulkanResource(VulkanResource&& VulkanResource)
		: VulkanDeviceChild(std::exchange(VulkanResource.Parent, {}))
		, AllocationDesc(std::exchange(VulkanResource.AllocationDesc, {}))
		, Allocation(std::exchange(VulkanResource.Allocation, {}))
	{
	}
	VulkanResource& operator=(VulkanResource&& VulkanResource)
	{
		if (this != &VulkanResource)
		{
			Parent		   = std::exchange(VulkanResource.Parent, {});
			AllocationDesc = std::exchange(VulkanResource.AllocationDesc, {});
			Allocation	   = std::exchange(VulkanResource.Allocation, {});
		}
		return *this;
	}

protected:
	VmaAllocationCreateInfo AllocationDesc = {};
	VmaAllocation			Allocation	   = VK_NULL_HANDLE;
};

class VulkanBuffer : public VulkanResource
{
public:
	VulkanBuffer() noexcept = default;
	VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanBuffer();

	VulkanBuffer(VulkanBuffer&& VulkanBuffer)
		: VulkanResource(std::forward<VulkanResource>(VulkanBuffer))
		, Buffer(std::exchange(VulkanBuffer.Buffer, VK_NULL_HANDLE))
		, Desc(std::exchange(VulkanBuffer.Desc, {}))
	{
	}
	VulkanBuffer& operator=(VulkanBuffer&& VulkanBuffer)
	{
		if (this != &VulkanBuffer)
		{
			VulkanResource::operator=(std::forward<VulkanResource>(VulkanBuffer));
			Buffer					= std::exchange(VulkanBuffer.Buffer, VK_NULL_HANDLE);
			Desc					= std::exchange(VulkanBuffer.Desc, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanBuffer);

	operator VkBuffer() const noexcept { return Buffer; }

	void Upload(std::function<void(void* CPUVirtualAddress)> Function);

private:
	VkBuffer		   Buffer = VK_NULL_HANDLE;
	VkBufferCreateInfo Desc	  = {};
};
