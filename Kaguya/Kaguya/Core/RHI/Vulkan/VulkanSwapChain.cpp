#include "VulkanSwapChain.h"

VulkanSwapChain::~VulkanSwapChain()
{
	vkDestroySemaphore(Device, RenderSemaphore, nullptr);
	vkDestroySemaphore(Device, PresentSemaphore, nullptr);

	for (auto ImageView : ImageViews)
	{
		vkDestroyImageView(Device, ImageView, nullptr);
	}

	vkDestroySwapchainKHR(Device, VkSwapchain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
}

void VulkanSwapChain::Initialize(HWND hWnd, VulkanDevice* Device)
{
	this->Instance = Device->GetVkInstance();
	this->Device   = Device->GetVkDevice();

	// Surface
	auto Win32SurfaceCreateInfo		 = VkStruct<VkWin32SurfaceCreateInfoKHR>();
	Win32SurfaceCreateInfo.hwnd		 = hWnd;
	Win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	VERIFY_VULKAN_API(vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, &Surface));

	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device->GetVkPhysicalDevice(), Surface, &SurfaceCapabilities));

	uint32_t SurfaceFormatCount = 0;
	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device->GetVkPhysicalDevice(), Surface, &SurfaceFormatCount, nullptr));
	if (SurfaceFormatCount != 0)
	{
		SurfaceFormats.resize(SurfaceFormatCount);
		VERIFY_VULKAN_API(vkGetPhysicalDeviceSurfaceFormatsKHR(
			Device->GetVkPhysicalDevice(),
			Surface,
			&SurfaceFormatCount,
			SurfaceFormats.data()));
	}

	uint32_t PresentModeCount = 0;
	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr));
	if (PresentModeCount != 0)
	{
		PresentModes.resize(PresentModeCount);
		VERIFY_VULKAN_API(vkGetPhysicalDeviceSurfacePresentModesKHR(
			Device->GetVkPhysicalDevice(),
			Surface,
			&PresentModeCount,
			PresentModes.data()));
	}

	auto SemaphoreCreateInfo  = VkStruct<VkSemaphoreCreateInfo>();
	SemaphoreCreateInfo.flags = 0;
	VERIFY_VULKAN_API(vkCreateSemaphore(Device->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &PresentSemaphore));
	VERIFY_VULKAN_API(vkCreateSemaphore(Device->GetVkDevice(), &SemaphoreCreateInfo, nullptr, &RenderSemaphore));

	PresentQueue								  = Device->InitializePresentQueue(Surface);
	PresentQueue->SubmitInfo.waitSemaphoreCount	  = 1;
	PresentQueue->SubmitInfo.pWaitSemaphores	  = &PresentSemaphore;
	PresentQueue->SubmitInfo.pWaitDstStageMask	  = &PipelineStageFlags;
	PresentQueue->SubmitInfo.signalSemaphoreCount = 1;
	PresentQueue->SubmitInfo.pSignalSemaphores	  = &RenderSemaphore;

	VkSurfaceFormatKHR SurfaceFormat = GetPreferredSurfaceFormat();
	VkPresentModeKHR   PresentMode	 = GetPreferredPresentMode();
	VkExtent2D		   Extent		 = GetPreferredExtent();
	uint32_t		   ImageCount	 = SurfaceCapabilities.minImageCount + 1;
	if (SurfaceCapabilities.maxImageCount > 0 && ImageCount > SurfaceCapabilities.maxImageCount)
	{
		ImageCount = SurfaceCapabilities.maxImageCount;
	}

	auto SwapchainCreateInfo			 = VkStruct<VkSwapchainCreateInfoKHR>();
	SwapchainCreateInfo.surface			 = Surface;
	SwapchainCreateInfo.minImageCount	 = ImageCount;
	SwapchainCreateInfo.imageFormat		 = SurfaceFormat.format;
	SwapchainCreateInfo.imageColorSpace	 = SurfaceFormat.colorSpace;
	SwapchainCreateInfo.imageExtent		 = Extent;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageUsage		 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// We force present queue to be the same as graphics queue
	SwapchainCreateInfo.imageSharingMode	  = VK_SHARING_MODE_EXCLUSIVE;
	SwapchainCreateInfo.queueFamilyIndexCount = 0;		 // Optional
	SwapchainCreateInfo.pQueueFamilyIndices	  = nullptr; // Optional
	SwapchainCreateInfo.preTransform		  = SurfaceCapabilities.currentTransform;
	SwapchainCreateInfo.compositeAlpha		  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainCreateInfo.presentMode			  = PresentMode;
	SwapchainCreateInfo.clipped				  = VK_TRUE;
	SwapchainCreateInfo.oldSwapchain		  = VK_NULL_HANDLE;

	VERIFY_VULKAN_API(vkCreateSwapchainKHR(Device->GetVkDevice(), &SwapchainCreateInfo, nullptr, &VkSwapchain));

	Format		 = SurfaceFormat.format;
	this->Extent = Extent;
	VERIFY_VULKAN_API(vkGetSwapchainImagesKHR(Device->GetVkDevice(), VkSwapchain, &ImageCount, nullptr));
	Images.resize(ImageCount);
	VERIFY_VULKAN_API(vkGetSwapchainImagesKHR(Device->GetVkDevice(), VkSwapchain, &ImageCount, Images.data()));

	ImageViews.resize(ImageCount);
	for (uint32_t i = 0; i < ImageCount; ++i)
	{
		auto ImageViewCreateInfo							= VkStruct<VkImageViewCreateInfo>();
		ImageViewCreateInfo.image							= Images[i];
		ImageViewCreateInfo.viewType						= VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format							= Format;
		ImageViewCreateInfo.components.r					= VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g					= VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b					= VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.a					= VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
		ImageViewCreateInfo.subresourceRange.levelCount		= 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount		= 1;

		VERIFY_VULKAN_API(vkCreateImageView(Device->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageViews[i]))
	}
}

auto VulkanSwapChain::GetImageView(size_t i) const noexcept -> VkImageView
{
	return ImageViews[i];
}

uint32_t VulkanSwapChain::AcquireNextImage()
{
	VERIFY_VULKAN_API(
		vkAcquireNextImageKHR(Device, VkSwapchain, UINT64_MAX, PresentSemaphore, nullptr, &CurrentImageIndex));
	return CurrentImageIndex;
}

void VulkanSwapChain::Present()
{
	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	const VkSwapchainKHR Swapchains[] = { VkSwapchain };
	auto				 PresentInfo  = VkStruct<VkPresentInfoKHR>();
	PresentInfo.waitSemaphoreCount	  = 1;
	PresentInfo.pWaitSemaphores		  = &RenderSemaphore;
	PresentInfo.swapchainCount		  = static_cast<uint32_t>(std::size(Swapchains));
	PresentInfo.pSwapchains			  = Swapchains;
	PresentInfo.pImageIndices		  = &CurrentImageIndex;

	VERIFY_VULKAN_API(vkQueuePresentKHR(PresentQueue->GetApiHandle(), &PresentInfo));
}

auto VulkanSwapChain::GetPreferredSurfaceFormat() const noexcept -> VkSurfaceFormatKHR
{
	for (const auto& SurfaceFormat : SurfaceFormats)
	{
		if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return SurfaceFormat;
		}
	}

	return SurfaceFormats[0];
}

auto VulkanSwapChain::GetPreferredPresentMode() const noexcept -> VkPresentModeKHR
{
	for (const auto& PresentMode : PresentModes)
	{
		if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return PresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

auto VulkanSwapChain::GetPreferredExtent() const noexcept -> VkExtent2D
{
	if (SurfaceCapabilities.currentExtent.width != UINT32_MAX && SurfaceCapabilities.currentExtent.height != UINT32_MAX)
	{
		return SurfaceCapabilities.currentExtent;
	}
	else
	{
		const int  Width		= Application::GetWidth();
		const int  Height		= Application::GetHeight();
		VkExtent2D ActualExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };

		ActualExtent.width = std::clamp(
			ActualExtent.width,
			SurfaceCapabilities.minImageExtent.width,
			SurfaceCapabilities.maxImageExtent.width);
		ActualExtent.height = std::clamp(
			ActualExtent.height,
			SurfaceCapabilities.minImageExtent.height,
			SurfaceCapabilities.maxImageExtent.height);
		return ActualExtent;
	}
}
