#include "VulkanCommandQueue.h"
#include "VulkanDevice.h"

VulkanCommandQueue::VulkanCommandQueue(VulkanDevice* Parent) noexcept
	: VulkanDeviceChild(Parent)
{
}

VulkanCommandQueue::~VulkanCommandQueue()
{
}

void VulkanCommandQueue::Initialize(uint32_t QueueFamilyIndex)
{
	this->QueueFamilyIndex = QueueFamilyIndex;

	vkGetDeviceQueue(GetParentDevice()->GetVkDevice(), QueueFamilyIndex, 0, &Queue);

	auto CommandPoolCreateInfo			   = VkStruct<VkCommandPoolCreateInfo>();
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	CommandPoolCreateInfo.flags			   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VERIFY_VULKAN_API(
		vkCreateCommandPool(GetParentDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

	// allocate the default command buffer that we will use for rendering
	auto CommandBufferAllocateInfo				 = VkStruct<VkCommandBufferAllocateInfo>();
	CommandBufferAllocateInfo.commandPool		 = CommandPool;
	CommandBufferAllocateInfo.commandBufferCount = 1;
	CommandBufferAllocateInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VERIFY_VULKAN_API(
		vkAllocateCommandBuffers(GetParentDevice()->GetVkDevice(), &CommandBufferAllocateInfo, &CommandBuffer));
}

void VulkanCommandQueue::Destroy()
{
	vkDestroyCommandPool(GetParentDevice()->GetVkDevice(), std::exchange(CommandPool, {}), nullptr);
}
