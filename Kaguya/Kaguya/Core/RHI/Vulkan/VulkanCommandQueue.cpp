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

void VulkanCommandQueue::WaitForFence(UINT64 FenceValue)
{
	if (IsFenceComplete(FenceValue))
	{
		return;
	}

	VkSemaphore Semaphores[] = { Fence.GetApiHandle() };

	auto SemaphoreWaitInfo			 = VkStruct<VkSemaphoreWaitInfo>();
	SemaphoreWaitInfo.flags			 = 0;
	SemaphoreWaitInfo.semaphoreCount = 1;
	SemaphoreWaitInfo.pSemaphores	 = Semaphores;
	SemaphoreWaitInfo.pValues		 = &FenceValue;

	VERIFY_VULKAN_API(vkWaitSemaphores(Parent->GetVkDevice(), &SemaphoreWaitInfo, UINT64_MAX));
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

	SubmitInfo.commandBufferCount = NumCommandLists;
	SubmitInfo.pCommandBuffers	  = CommandLists;
	VERIFY_VULKAN_API(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));

	UINT64 FenceValue = AdvanceGpu();
	SyncPoint		  = VulkanCommandSyncPoint(&Fence, FenceValue);

	return SyncPoint;
}

void VulkanFence::Initialize(UINT64 InitialValue)
{
	auto SemaphoreTypeCreateInfo		  = VkStruct<VkSemaphoreTypeCreateInfo>();
	SemaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	SemaphoreTypeCreateInfo.initialValue  = InitialValue;

	auto SemaphoreCreateInfo  = VkStruct<VkSemaphoreCreateInfo>();
	SemaphoreCreateInfo.pNext = &SemaphoreTypeCreateInfo;
	SemaphoreCreateInfo.flags = 0;

	VERIFY_VULKAN_API(vkCreateSemaphore(Parent->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &Semaphore));
}

void VulkanFence::Destroy()
{
	vkDestroySemaphore(Parent->GetVkDevice(), std::exchange(Semaphore, {}), nullptr);
}

UINT64 VulkanFence::GetCompletedValue() const
{
	UINT64 Value = 0;
	VERIFY_VULKAN_API(vkGetSemaphoreCounterValue(Parent->GetVkDevice(), Semaphore, &Value));
	return Value;
}

void VulkanFence::Signal(UINT64 Value)
{
	auto SemaphoreSignalInfo	  = VkStruct<VkSemaphoreSignalInfo>();
	SemaphoreSignalInfo.semaphore = Semaphore;
	SemaphoreSignalInfo.value	  = Value;
	VERIFY_VULKAN_API(vkSignalSemaphore(Parent->GetVkDevice(), &SemaphoreSignalInfo));
}
