#pragma once
#include "VulkanCommon.h"

class CommandQueue : public DeviceChild
{
public:
	CommandQueue(Device* Parent) noexcept;
	~CommandQueue();

	void Initialize(uint32_t QueueFamilyIndex);
	void Destroy();

	[[nodiscard]] auto GetAPIHandle() const noexcept -> VkQueue { return Queue; }
	[[nodiscard]] auto GetQueueFamilyIndex() const noexcept -> uint32_t { return QueueFamilyIndex; }
	[[nodiscard]] auto GetCommandBuffer() const noexcept -> VkCommandBuffer { return CommandBuffer; }

private:
	VkQueue	 Queue = VK_NULL_HANDLE;
	uint32_t QueueFamilyIndex;

	VkCommandPool CommandPool = VK_NULL_HANDLE;

	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
};
