#include "VulkanCommandQueue.h"
#include "VulkanDevice.h"

VulkanCommandQueue::VulkanCommandQueue(VulkanDevice* Parent) noexcept
	: VulkanDeviceChild(Parent)
	, Fence(Parent)
{
}

VulkanCommandQueue::~VulkanCommandQueue()
{
}

void VulkanCommandQueue::Initialize(uint32_t QueueFamilyIndex, uint32_t NumCommandLists /*= 1*/)
{
	this->QueueFamilyIndex = QueueFamilyIndex;

	vkGetDeviceQueue(GetParentDevice()->GetVkDevice(), QueueFamilyIndex, 0, &Queue);

	// Create timeline semaphore (ID3D12Fence)
	Fence.Initialize(0);

	// Create command pool
	auto CommandPoolCreateInfo			   = VkStruct<VkCommandPoolCreateInfo>();
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	CommandPoolCreateInfo.flags			   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VERIFY_VULKAN_API(
		vkCreateCommandPool(GetParentDevice()->GetVkDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

	// Allocate command buffers
	auto CommandBufferAllocateInfo				 = VkStruct<VkCommandBufferAllocateInfo>();
	CommandBufferAllocateInfo.commandPool		 = CommandPool;
	CommandBufferAllocateInfo.commandBufferCount = NumCommandLists;
	CommandBufferAllocateInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	std::vector<VkCommandBuffer> CommandBuffers(NumCommandLists);

	VERIFY_VULKAN_API(
		vkAllocateCommandBuffers(GetParentDevice()->GetVkDevice(), &CommandBufferAllocateInfo, CommandBuffers.data()));

	CommandContexts.reserve(NumCommandLists);
	for (auto CommandBuffer : CommandBuffers)
	{
		VulkanCommandContext& Context = CommandContexts.emplace_back(GetParentDevice());
		Context.CommandBuffer		  = CommandBuffer;
	}
}

void VulkanCommandQueue::Destroy()
{
	Fence.Destroy();
	vkDestroyCommandPool(GetParentDevice()->GetVkDevice(), std::exchange(CommandPool, {}), nullptr);
}

UINT64 VulkanCommandQueue::AdvanceGpu()
{
	std::scoped_lock _(FenceMutex);

	auto TimelineSemaphoreSubmitInfo					  = VkStruct<VkTimelineSemaphoreSubmitInfo>();
	TimelineSemaphoreSubmitInfo.waitSemaphoreValueCount	  = 0;
	TimelineSemaphoreSubmitInfo.pWaitSemaphoreValues	  = nullptr;
	TimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = 1;
	TimelineSemaphoreSubmitInfo.pSignalSemaphoreValues	  = &FenceValue;

	const VkSemaphore SignalSemaphores[] = { Fence.GetApiHandle() };

	auto SubmitInfo					= VkStruct<VkSubmitInfo>();
	SubmitInfo.pNext				= &TimelineSemaphoreSubmitInfo;
	SubmitInfo.waitSemaphoreCount	= 0;
	SubmitInfo.pWaitSemaphores		= nullptr;
	SubmitInfo.pWaitDstStageMask	= nullptr;
	SubmitInfo.commandBufferCount	= 0;
	SubmitInfo.pCommandBuffers		= nullptr;
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores	= SignalSemaphores;

	VERIFY_VULKAN_API(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
	return FenceValue++;
}

bool VulkanCommandQueue::IsFenceComplete(UINT64 FenceValue) const
{
	return Fence.GetCompletedValue() >= FenceValue;
}

void VulkanCommandQueue::HostWaitForValue(UINT64 FenceValue)
{
	if (IsFenceComplete(FenceValue))
	{
		return;
	}

	Fence.Wait(FenceValue);
}

void VulkanCommandQueue::Wait(VulkanCommandQueue* CommandQueue)
{
	WaitForSyncPoint(CommandQueue->SyncPoint);
}

void VulkanCommandQueue::WaitForSyncPoint(const VulkanCommandSyncPoint& SyncPoint)
{
	if (SyncPoint.IsValid())
	{
		const uint64_t	  WaitSemaphoreValues[] = { SyncPoint.GetValue() };
		const VkSemaphore WaitSemaphores[]		= { Fence.GetApiHandle() };

		auto TimelineSemaphoreSubmitInfo					= VkStruct<VkTimelineSemaphoreSubmitInfo>();
		TimelineSemaphoreSubmitInfo.waitSemaphoreValueCount = 1;
		TimelineSemaphoreSubmitInfo.pWaitSemaphoreValues	= WaitSemaphoreValues;

		auto SubmitInfo				  = VkStruct<VkSubmitInfo>();
		SubmitInfo.pNext			  = &TimelineSemaphoreSubmitInfo;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores	  = WaitSemaphores;

		VERIFY_VULKAN_API(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
	}
}

VulkanCommandSyncPoint VulkanCommandQueue::ExecuteCommandLists(
	UINT				  NumCommandListHandles,
	VulkanCommandContext* CommandListHandles,
	bool				  WaitForCompletion)
{
	UINT			NumCommandLists	 = 0;
	VkCommandBuffer CommandLists[64] = {};

	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		CommandLists[NumCommandLists++] = CommandListHandles[i].CommandBuffer;
	}

	constexpr VkPipelineStageFlags Stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	SubmitInfo.pWaitDstStageMask  = &Stage;
	SubmitInfo.commandBufferCount = NumCommandLists;
	SubmitInfo.pCommandBuffers	  = CommandLists;
	VERIFY_VULKAN_API(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

	UINT64 FenceValue = AdvanceGpu();
	SyncPoint		  = VulkanCommandSyncPoint(&Fence, FenceValue);

	return SyncPoint;
}
