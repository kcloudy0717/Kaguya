#include "SwapChain.h"

SwapChain::~SwapChain()
{
	for (auto ImageView : ImageViews)
	{
		vkDestroyImageView(Device, ImageView, nullptr);
	}

	vkDestroySwapchainKHR(Device, VkSwapchain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
}

void SwapChain::Initialize(HWND hWnd, Adapter* Adapter)
{
	this->Instance = Adapter->GetVkInstance();
	this->Device   = Adapter->GetVkDevice();

	// Surface
	auto Win32SurfaceCreateInfo		 = VkStruct<VkWin32SurfaceCreateInfoKHR>();
	Win32SurfaceCreateInfo.hwnd		 = hWnd;
	Win32SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	VERIFY_VULKAN_API(vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, &Surface));

	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Adapter->GetVkPhysicalDevice(), Surface, &SurfaceCapabilities));

	uint32_t SurfaceFormatCount = 0;
	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfaceFormatsKHR(Adapter->GetVkPhysicalDevice(), Surface, &SurfaceFormatCount, nullptr));
	if (SurfaceFormatCount != 0)
	{
		SurfaceFormats.resize(SurfaceFormatCount);
		VERIFY_VULKAN_API(vkGetPhysicalDeviceSurfaceFormatsKHR(
			Adapter->GetVkPhysicalDevice(),
			Surface,
			&SurfaceFormatCount,
			SurfaceFormats.data()));
	}

	uint32_t PresentModeCount = 0;
	VERIFY_VULKAN_API(
		vkGetPhysicalDeviceSurfacePresentModesKHR(Adapter->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr));
	if (PresentModeCount != 0)
	{
		PresentModes.resize(PresentModeCount);
		VERIFY_VULKAN_API(vkGetPhysicalDeviceSurfacePresentModesKHR(
			Adapter->GetVkPhysicalDevice(),
			Surface,
			&PresentModeCount,
			PresentModes.data()));
	}

	PresentQueue = Adapter->InitializePresentQueue(Surface);

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

	VERIFY_VULKAN_API(vkCreateSwapchainKHR(Adapter->GetVkDevice(), &SwapchainCreateInfo, nullptr, &VkSwapchain));

	Format		 = SurfaceFormat.format;
	this->Extent = Extent;
	VERIFY_VULKAN_API(vkGetSwapchainImagesKHR(Device, VkSwapchain, &ImageCount, nullptr));
	Images.resize(ImageCount);
	VERIFY_VULKAN_API(vkGetSwapchainImagesKHR(Device, VkSwapchain, &ImageCount, Images.data()));

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

		VERIFY_VULKAN_API(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ImageViews[i]))
	}
}

auto SwapChain::GetImageView(size_t i) const noexcept -> VkImageView
{
	return ImageViews[i];
}

VkSurfaceFormatKHR SwapChain::GetPreferredSurfaceFormat()
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

VkPresentModeKHR SwapChain::GetPreferredPresentMode()
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

VkExtent2D SwapChain::GetPreferredExtent()
{
	if (SurfaceCapabilities.currentExtent.width != UINT32_MAX)
	{
		return SurfaceCapabilities.currentExtent;
	}
	else
	{
		int width = Application::GetWidth(), height = Application::GetHeight();

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::clamp(
			actualExtent.width,
			SurfaceCapabilities.minImageExtent.width,
			SurfaceCapabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(
			actualExtent.height,
			SurfaceCapabilities.minImageExtent.height,
			SurfaceCapabilities.maxImageExtent.height);

		return actualExtent;
	}
}
