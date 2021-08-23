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

	NONCOPYABLE(VulkanResource);

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
		, Desc(std::exchange(VulkanBuffer.Desc, {}))
		, Buffer(std::exchange(VulkanBuffer.Buffer, VK_NULL_HANDLE))
	{
	}
	VulkanBuffer& operator=(VulkanBuffer&& VulkanBuffer)
	{
		if (this != &VulkanBuffer)
		{
			VulkanResource::operator=(std::forward<VulkanResource>(VulkanBuffer));
			Desc					= std::exchange(VulkanBuffer.Desc, {});
			Buffer					= std::exchange(VulkanBuffer.Buffer, VK_NULL_HANDLE);
		}
		return *this;
	}

	NONCOPYABLE(VulkanBuffer);

	operator VkBuffer() const noexcept { return Buffer; }

	void Upload(std::function<void(void* CPUVirtualAddress)> Function);

private:
	VkBufferCreateInfo Desc	  = {};
	VkBuffer		   Buffer = VK_NULL_HANDLE;
};

inline VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
	auto info	   = VkStruct<VkImageCreateInfo>();
	info.imageType = VK_IMAGE_TYPE_2D;

	info.format = format;
	info.extent = extent;

	info.mipLevels	 = 1;
	info.arrayLayers = 1;
	info.samples	 = VK_SAMPLE_COUNT_1_BIT;
	info.tiling		 = VK_IMAGE_TILING_OPTIMAL;
	info.usage		 = usageFlags;

	return info;
}

inline VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
{
	// build a image-view for the depth image to use for rendering
	auto info							 = VkStruct<VkImageViewCreateInfo>();
	info.viewType						 = VK_IMAGE_VIEW_TYPE_2D;
	info.image							 = image;
	info.format							 = format;
	info.subresourceRange.baseMipLevel	 = 0;
	info.subresourceRange.levelCount	 = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount	 = 1;
	info.subresourceRange.aspectMask	 = aspectFlags;

	return info;
}

class VulkanTexture : public VulkanResource
{
public:
	VulkanTexture() noexcept = default;
	VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanTexture();

	VulkanTexture(VulkanTexture&& VulkanTexture) noexcept
		: VulkanResource(std::forward<VulkanResource>(VulkanTexture))
		, Desc(std::exchange(VulkanTexture.Desc, {}))
		, Texture(std::exchange(VulkanTexture.Texture, VK_NULL_HANDLE))
	{
	}
	VulkanTexture& operator=(VulkanTexture&& VulkanTexture) noexcept
	{
		if (this != &VulkanTexture)
		{
			VulkanResource::operator=(std::forward<VulkanResource>(VulkanTexture));
			Desc					= std::exchange(VulkanTexture.Desc, {});
			Texture					= std::exchange(VulkanTexture.Texture, VK_NULL_HANDLE);
		}
		return *this;
	}

	NONCOPYABLE(VulkanTexture);

	[[nodiscard]] auto GetDesc() const noexcept -> const VkImageCreateInfo& { return Desc; }

	operator VkImage() const noexcept { return Texture; }

private:
	VkImageCreateInfo Desc	  = {};
	VkImage			  Texture = VK_NULL_HANDLE;
};
