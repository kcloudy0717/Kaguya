#pragma once
#include "VulkanCommon.h"
#include "VulkanCommandContext.h"

class VulkanFence final : public VulkanDeviceChild
{
public:
	using VulkanDeviceChild::VulkanDeviceChild;

	[[nodiscard]] auto GetApiHandle() const noexcept -> VkSemaphore { return Semaphore; }

	void Initialize(UINT64 InitialValue);
	void Destroy();

	UINT64 GetCompletedValue() const;

	void Signal(UINT64 Value);

private:
	VkSemaphore Semaphore = VK_NULL_HANDLE;
};

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

	void WaitForFence(UINT64 FenceValue);

	void Wait(VulkanCommandQueue* CommandQueue);
	void WaitForSyncPoint(const VulkanCommandSyncPoint& SyncPoint);

	void Flush() { WaitForFence(AdvanceGpu()); }

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

	VkSubmitInfo SubmitInfo = VkStruct<VkSubmitInfo>();

	VkCommandPool					  CommandPool = VK_NULL_HANDLE;
	std::vector<VulkanCommandContext> CommandContexts;
};
