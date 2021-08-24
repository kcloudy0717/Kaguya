#pragma once
#include "VulkanCommon.h"

class VulkanBuffer : public IRHIBuffer
{
public:
	VulkanBuffer() noexcept = default;
	VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanBuffer();

	auto GetParentDevice() noexcept -> VulkanDevice*;

	VulkanBuffer(VulkanBuffer&& VulkanBuffer)
		: IRHIBuffer(std::exchange(VulkanBuffer.Parent, {}))
		, AllocationDesc(std::exchange(VulkanBuffer.AllocationDesc, {}))
		, Allocation(std::exchange(VulkanBuffer.Allocation, {}))
		, Desc(std::exchange(VulkanBuffer.Desc, {}))
		, Buffer(std::exchange(VulkanBuffer.Buffer, {}))
	{
	}
	VulkanBuffer& operator=(VulkanBuffer&& VulkanBuffer)
	{
		if (this != &VulkanBuffer)
		{
			Parent		   = std::exchange(VulkanBuffer.Parent, {});
			AllocationDesc = std::exchange(VulkanBuffer.AllocationDesc, {});
			Allocation	   = std::exchange(VulkanBuffer.Allocation, {});
			Desc		   = std::exchange(VulkanBuffer.Desc, {});
			Buffer		   = std::exchange(VulkanBuffer.Buffer, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanBuffer);

	VkBuffer GetApiHandle() const noexcept { return Buffer; }

	void Upload(std::function<void(void* CPUVirtualAddress)> Function);

private:
	VmaAllocationCreateInfo AllocationDesc = {};
	VmaAllocation			Allocation	   = VK_NULL_HANDLE;
	VkBufferCreateInfo		Desc		   = {};
	VkBuffer				Buffer		   = VK_NULL_HANDLE;
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

class VulkanTexture : public IRHITexture
{
public:
	VulkanTexture() noexcept = default;
	VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanTexture();

	auto GetParentDevice() noexcept -> VulkanDevice*;

	VulkanTexture(VulkanTexture&& VulkanTexture) noexcept
		: IRHITexture(std::exchange(VulkanTexture.Parent, {}))
		, AllocationDesc(std::exchange(VulkanTexture.AllocationDesc, {}))
		, Allocation(std::exchange(VulkanTexture.Allocation, {}))
		, Desc(std::exchange(VulkanTexture.Desc, {}))
		, Texture(std::exchange(VulkanTexture.Texture, {}))
	{
	}
	VulkanTexture& operator=(VulkanTexture&& VulkanTexture) noexcept
	{
		if (this != &VulkanTexture)
		{
			Parent		   = std::exchange(VulkanTexture.Parent, {});
			AllocationDesc = std::exchange(VulkanTexture.AllocationDesc, {});
			Allocation	   = std::exchange(VulkanTexture.Allocation, {});
			Desc		   = std::exchange(VulkanTexture.Desc, {});
			Texture		   = std::exchange(VulkanTexture.Texture, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanTexture);

	[[nodiscard]] auto GetDesc() const noexcept -> const VkImageCreateInfo& { return Desc; }

	operator VkImage() const noexcept { return Texture; }

private:
	VmaAllocationCreateInfo AllocationDesc = {};
	VmaAllocation			Allocation	   = VK_NULL_HANDLE;
	VkImageCreateInfo		Desc		   = {};
	VkImage					Texture		   = VK_NULL_HANDLE;
};
