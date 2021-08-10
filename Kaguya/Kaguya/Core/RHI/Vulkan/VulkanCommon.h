#pragma once

class VulkanException : public CoreException
{
public:
	VulkanException(const char* File, int Line, VkResult ErrorCode)
		: CoreException(File, Line)
		, ErrorCode(ErrorCode)
	{
	}

	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	const VkResult ErrorCode;
};

#define VERIFY_VULKAN_API(expr)                                                                                        \
	{                                                                                                                  \
		VkResult result = expr;                                                                                        \
		if (result)                                                                                                    \
		{                                                                                                              \
			throw VulkanException(__FILE__, __LINE__, result);                                                         \
		}                                                                                                              \
	}

class Adapter;

class AdapterChild
{
public:
	AdapterChild() noexcept
		: Parent(nullptr)
	{
	}

	AdapterChild(Adapter* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentAdapter() const noexcept -> Adapter* { return Parent; }

	void SetParentAdapter(Adapter* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	Adapter* Parent;
};

class Device;

class DeviceChild
{
public:
	DeviceChild() noexcept
		: Parent(nullptr)
	{
	}

	DeviceChild(Device* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentDevice() const noexcept -> Device* { return Parent; }

	void SetParentDevice(Device* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	Device* Parent;
};

// clang-format off
template<typename T>	inline [[nodiscard]] auto VkStruct() -> T									{ static_assert("Not implemented"); return T(); }
template<>				inline [[nodiscard]] auto VkStruct() -> VkApplicationInfo					{ return { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkInstanceCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDeviceQueueCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDeviceCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkSubmitInfo						{ return { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkFenceCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkSemaphoreCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkImageViewCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkFramebufferCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkRenderPassCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandPoolCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandBufferAllocateInfo			{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandBufferBeginInfo			{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkRenderPassBeginInfo				{ return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkSwapchainCreateInfoKHR			{ return { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPresentInfoKHR					{ return { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkWin32SurfaceCreateInfoKHR			{ return { .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkDebugUtilsMessengerCreateInfoEXT	{ return { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT }; }
// clang-format on

struct QueueFamilyIndices
{
	bool IsValid() { return GraphicsFamily.has_value(); }

	std::optional<uint32_t> GraphicsFamily;
};
