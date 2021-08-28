#pragma once
#include "VulkanCommon.h"

class VulkanFence final : public VulkanDeviceChild
{
public:
	using VulkanDeviceChild::VulkanDeviceChild;

	[[nodiscard]] auto GetApiHandle() const noexcept -> VkSemaphore { return Semaphore; }

	void Initialize(UINT64 InitialValue);
	void Destroy();

	UINT64 GetCompletedValue() const;

	void Wait(UINT64 Value);

	void Signal(UINT64 Value);

private:
	// Timeline Semaphore
	VkSemaphore Semaphore = VK_NULL_HANDLE;
};
