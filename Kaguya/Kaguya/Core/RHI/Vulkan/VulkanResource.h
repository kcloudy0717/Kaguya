#pragma once
#include "VulkanCommon.h"

class VulkanBuffer final : public IRHIBuffer
{
public:
	VulkanBuffer() noexcept = default;
	VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanBuffer() override;

	VkBufferCreateInfo GetDesc() const noexcept { return Desc; }
	VkBuffer		   GetApiHandle() const noexcept { return Buffer; }

	void Upload(std::function<void(void* CPUVirtualAddress)> Function);

	VmaAllocationCreateInfo AllocationDesc = {};
	VmaAllocation			Allocation	   = VK_NULL_HANDLE;
	VkBufferCreateInfo		Desc		   = {};
	VkBuffer				Buffer		   = VK_NULL_HANDLE;
};

class VulkanTexture final : public IRHITexture
{
public:
	VulkanTexture() noexcept = default;
	VulkanTexture(VulkanDevice* Parent, VkImageCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanTexture() override;

	VkImageCreateInfo GetDesc() const noexcept { return Desc; }
	VkImage			  GetApiHandle() const noexcept { return Texture; }
	VkImageView		  GetImageView() const noexcept { return ImageView; }

	VmaAllocationCreateInfo					AllocationDesc = {};
	VmaAllocation							Allocation	   = VK_NULL_HANDLE;
	VkImageCreateInfo						Desc		   = {};
	VkImage									Texture		   = VK_NULL_HANDLE;
	VkImageView								ImageView	   = VK_NULL_HANDLE;
	std::unordered_map<UINT64, VkImageView> ImageViewTable;
};
