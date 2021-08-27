#pragma once
#include "VulkanCommon.h"

class VulkanBuffer final : public IRHIBuffer
{
public:
	VulkanBuffer() noexcept = default;
	VulkanBuffer(VulkanDevice* Parent, VkBufferCreateInfo Desc, VmaAllocationCreateInfo AllocationDesc);
	~VulkanBuffer() override;

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

	VulkanTexture(VulkanTexture&& VulkanTexture) noexcept
		: IRHITexture(std::exchange(VulkanTexture.Parent, {}))
		, AllocationDesc(std::exchange(VulkanTexture.AllocationDesc, {}))
		, Allocation(std::exchange(VulkanTexture.Allocation, {}))
		, Desc(std::exchange(VulkanTexture.Desc, {}))
		, Texture(std::exchange(VulkanTexture.Texture, {}))
		, ImageView(std::exchange(VulkanTexture.ImageView, {}))
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
			ImageView	   = std::exchange(VulkanTexture.ImageView, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanTexture);

	VkImageCreateInfo GetDesc() const noexcept { return Desc; }
	VkImage			  GetApiHandle() const noexcept { return Texture; }
	VkImageView		  GetImageView() const noexcept { return ImageView; }

	VmaAllocationCreateInfo AllocationDesc = {};
	VmaAllocation			Allocation	   = VK_NULL_HANDLE;
	VkImageCreateInfo		Desc		   = {};
	VkImage					Texture		   = VK_NULL_HANDLE;
	VkImageView				ImageView	   = VK_NULL_HANDLE;
};
