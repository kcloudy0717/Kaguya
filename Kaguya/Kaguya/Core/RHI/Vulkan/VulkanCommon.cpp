#include "VulkanCommon.h"

const char* VulkanException::GetErrorType() const noexcept
{
	return "[Vulkan]";
}

std::string VulkanException::GetError() const
{
#define VKERR(x)                                                                                                       \
	case x:                                                                                                            \
		Error = #x;                                                                                                    \
		break

	std::string Error;
	switch (ErrorCode)
	{
		VKERR(VK_NOT_READY);
		VKERR(VK_TIMEOUT);
		VKERR(VK_EVENT_SET);
		VKERR(VK_EVENT_RESET);
		VKERR(VK_INCOMPLETE);
		VKERR(VK_ERROR_OUT_OF_HOST_MEMORY);
		VKERR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		VKERR(VK_ERROR_INITIALIZATION_FAILED);
		VKERR(VK_ERROR_DEVICE_LOST);
		VKERR(VK_ERROR_MEMORY_MAP_FAILED);
		VKERR(VK_ERROR_LAYER_NOT_PRESENT);
		VKERR(VK_ERROR_EXTENSION_NOT_PRESENT);
		VKERR(VK_ERROR_FEATURE_NOT_PRESENT);
		VKERR(VK_ERROR_INCOMPATIBLE_DRIVER);
		VKERR(VK_ERROR_TOO_MANY_OBJECTS);
		VKERR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		VKERR(VK_ERROR_FRAGMENTED_POOL);
		VKERR(VK_ERROR_UNKNOWN);
		VKERR(VK_ERROR_OUT_OF_POOL_MEMORY);
		VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
		VKERR(VK_ERROR_FRAGMENTATION);
		VKERR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
		VKERR(VK_ERROR_SURFACE_LOST_KHR);
		VKERR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
		VKERR(VK_SUBOPTIMAL_KHR);
		VKERR(VK_ERROR_OUT_OF_DATE_KHR);
		VKERR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
		VKERR(VK_ERROR_VALIDATION_FAILED_EXT);
		VKERR(VK_ERROR_INVALID_SHADER_NV);
		VKERR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
		VKERR(VK_ERROR_NOT_PERMITTED_EXT);
		VKERR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
		VKERR(VK_THREAD_IDLE_KHR);
		VKERR(VK_THREAD_DONE_KHR);
		VKERR(VK_OPERATION_DEFERRED_KHR);
		VKERR(VK_OPERATION_NOT_DEFERRED_KHR);
		VKERR(VK_PIPELINE_COMPILE_REQUIRED_EXT);

		// VKERR(VK_ERROR_OUT_OF_POOL_MEMORY_KHR);
		// VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR);
		// VKERR(VK_ERROR_FRAGMENTATION_EXT);
		// VKERR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
		// VKERR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
		// VKERR(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
	default:
	{
		char Buffer[64] = {};
		sprintf_s(Buffer, "VkResult of 0x%08X", static_cast<UINT>(ErrorCode));
		Error = Buffer;
	}
	break;
	}
#undef VKERR
	return Error;
}

auto VulkanCommandSyncPoint::IsValid() const noexcept -> bool
{
	return Fence != nullptr;
}

auto VulkanCommandSyncPoint::GetValue() const noexcept -> UINT64
{
	assert(IsValid());
	return Value;
}

auto VulkanCommandSyncPoint::IsComplete() const -> bool
{
	assert(IsValid());
	return Fence->GetCompletedValue() >= Value;
}

auto VulkanCommandSyncPoint::WaitForCompletion() const -> void
{
	assert(IsValid());
	const VkSemaphore Semaphores[] = { Fence->GetApiHandle() };

	auto SemaphoreWaitInfo			 = VkStruct<VkSemaphoreWaitInfo>();
	SemaphoreWaitInfo.flags			 = 0;
	SemaphoreWaitInfo.semaphoreCount = 1;
	SemaphoreWaitInfo.pSemaphores	 = Semaphores;
	SemaphoreWaitInfo.pValues		 = &Value;

	VERIFY_VULKAN_API(vkWaitSemaphores(Fence->GetParentDevice()->GetVkDevice(), &SemaphoreWaitInfo, UINT64_MAX));
}