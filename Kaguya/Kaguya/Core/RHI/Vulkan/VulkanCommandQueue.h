#pragma once
#include "VulkanCommon.h"
#include "VulkanFence.h"
#include "VulkanCommandContext.h"

class VulkanCommandQueue final : public VulkanDeviceChild
{
public:
	explicit VulkanCommandQueue(VulkanDevice* Parent) noexcept;
	~VulkanCommandQueue();

	[[nodiscard]] auto GetApiHandle() const noexcept -> VkQueue { return Queue; }
	[[nodiscard]] auto GetQueueFamilyIndex() const noexcept -> uint32_t { return QueueFamilyIndex; }
	[[nodiscard]] auto GetCommandPool() const noexcept -> VkCommandPool { return CommandPool; }
	[[nodiscard]] auto GetCommandContext(size_t Index) noexcept -> VulkanCommandContext&
	{
		return CommandContexts[Index];
	}

	void Initialize(uint32_t QueueFamilyIndex, uint32_t NumCommandLists = 1);
	void Destroy();

	[[nodiscard]] UINT64 AdvanceGpu();

	[[nodiscard]] bool IsFenceComplete(UINT64 FenceValue) const;

	void HostWaitForValue(UINT64 FenceValue);

	void Wait(VulkanCommandQueue* CommandQueue);
	void WaitForSyncPoint(const VulkanCommandSyncPoint& SyncPoint);

	void Flush() { HostWaitForValue(AdvanceGpu()); }

	VulkanCommandSyncPoint ExecuteCommandLists(
		UINT				  NumCommandListHandles,
		VulkanCommandContext* CommandListHandles,
		bool				  WaitForCompletion);

	uint32_t QueueFamilyIndex = UINT_MAX;

	VkQueue				   Queue = VK_NULL_HANDLE;
	VulkanFence			   Fence;
	CriticalSection		   FenceMutex;
	UINT64				   FenceValue = 1;
	VulkanCommandSyncPoint SyncPoint;

	// The wait/signal semaphore will be filled when there is a swapchain and this queue happens
	// to be a present queue
	VkSubmitInfo SubmitInfo = VkStruct<VkSubmitInfo>();

	VkCommandPool					  CommandPool = VK_NULL_HANDLE;
	std::vector<VulkanCommandContext> CommandContexts;
};
