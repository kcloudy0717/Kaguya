#pragma once
#include "VulkanCommon.h"

class VulkanCommandQueue final : public VulkanDeviceChild
{
public:
	explicit VulkanCommandQueue(VulkanDevice* Parent) noexcept;
	~VulkanCommandQueue();

	void Initialize(uint32_t QueueFamilyIndex);
	void Destroy();

	[[nodiscard]] auto GetApiHandle() const noexcept -> VkQueue { return Queue; }
	[[nodiscard]] auto GetQueueFamilyIndex() const noexcept -> uint32_t { return QueueFamilyIndex; }
	[[nodiscard]] auto GetCommandPool() const noexcept -> VkCommandPool { return CommandPool; }
	[[nodiscard]] auto GetCommandBuffer() const noexcept -> VkCommandBuffer { return CommandBuffer; }

private:
	VkQueue			Queue			 = VK_NULL_HANDLE;
	uint32_t		QueueFamilyIndex = UINT_MAX;
	VkCommandPool	CommandPool		 = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer	 = VK_NULL_HANDLE;
};
