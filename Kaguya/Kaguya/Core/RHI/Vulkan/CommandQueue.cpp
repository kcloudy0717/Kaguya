#include "CommandQueue.h"
#include "Device.h"

CommandQueue::CommandQueue(Device* Parent) noexcept
	: DeviceChild(Parent)
{
}

CommandQueue::~CommandQueue()
{
}

void CommandQueue::Initialize(uint32_t QueueFamilyIndex)
{
	this->QueueFamilyIndex = QueueFamilyIndex;

	vkGetDeviceQueue(GetParentDevice()->GetDevice(), QueueFamilyIndex, 0, &Queue);

	auto CommandPoolCreateInfo			   = VkStruct<VkCommandPoolCreateInfo>();
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	CommandPoolCreateInfo.flags			   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VERIFY_VULKAN_API(
		vkCreateCommandPool(GetParentDevice()->GetDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

	// allocate the default command buffer that we will use for rendering
	auto CommandBufferAllocateInfo				 = VkStruct<VkCommandBufferAllocateInfo>();
	CommandBufferAllocateInfo.commandPool		 = CommandPool;
	CommandBufferAllocateInfo.commandBufferCount = 1;
	CommandBufferAllocateInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VERIFY_VULKAN_API(
		vkAllocateCommandBuffers(GetParentDevice()->GetDevice(), &CommandBufferAllocateInfo, &CommandBuffer));
}

void CommandQueue::Destroy()
{
	vkDestroyCommandPool(GetParentDevice()->GetDevice(), CommandPool, nullptr);
}
