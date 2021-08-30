#include "VulkanFence.h"

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

void VulkanFence::Wait(UINT64 Value)
{
	auto SemaphoreWaitInfo			 = VkStruct<VkSemaphoreWaitInfo>();
	SemaphoreWaitInfo.flags			 = 0;
	SemaphoreWaitInfo.semaphoreCount = 1;
	SemaphoreWaitInfo.pSemaphores	 = &Semaphore;
	SemaphoreWaitInfo.pValues		 = &Value;
	VERIFY_VULKAN_API(vkWaitSemaphores(Parent->GetVkDevice(), &SemaphoreWaitInfo, UINT64_MAX));
}

void VulkanFence::Signal(UINT64 Value)
{
	auto SemaphoreSignalInfo	  = VkStruct<VkSemaphoreSignalInfo>();
	SemaphoreSignalInfo.semaphore = Semaphore;
	SemaphoreSignalInfo.value	  = Value;
	VERIFY_VULKAN_API(vkSignalSemaphore(Parent->GetVkDevice(), &SemaphoreSignalInfo));
}
