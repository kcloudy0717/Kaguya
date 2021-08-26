#pragma once
#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanSwapChain
{
public:
	VulkanSwapChain() noexcept = default;
	~VulkanSwapChain();

	void Initialize(HWND hWnd, VulkanDevice* Device);

	[[nodiscard]]	   operator auto() const noexcept { return VkSwapchain; }
	[[nodiscard]] auto GetFormat() const noexcept -> VkFormat { return Format; }
	[[nodiscard]] auto GetExtent() const noexcept -> VkExtent2D { return Extent; }
	[[nodiscard]] auto GetWidth() const noexcept -> uint32_t { return Extent.width; }
	[[nodiscard]] auto GetHeight() const noexcept -> uint32_t { return Extent.height; }
	[[nodiscard]] auto GetImageCount() const noexcept -> uint32_t { return static_cast<uint32_t>(Images.size()); }
	[[nodiscard]] auto GetImageView(size_t i) const noexcept -> VkImageView;

	uint32_t AcquireNextImage(VkSemaphore Semaphore)
	{
		VERIFY_VULKAN_API(
			vkAcquireNextImageKHR(Device, VkSwapchain, 1000000000, Semaphore, nullptr, &CurrentImageIndex));
		return CurrentImageIndex;
	}

	void Present(VkSemaphore RenderSemaphore)
	{
		// this will put the image we just rendered into the visible window.
		// we want to wait on the _renderSemaphore for that,
		// as it's necessary that drawing commands have finished before the image is displayed to the user
		VkSwapchainKHR Swapchains[]	   = { VkSwapchain };
		auto		   PresentInfo	   = VkStruct<VkPresentInfoKHR>();
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores	   = &RenderSemaphore;
		PresentInfo.swapchainCount	   = static_cast<uint32_t>(std::size(Swapchains));
		PresentInfo.pSwapchains		   = Swapchains;
		PresentInfo.pImageIndices	   = &CurrentImageIndex;

		VERIFY_VULKAN_API(vkQueuePresentKHR(PresentQueue->GetApiHandle(), &PresentInfo));
	}

private:
	VkSurfaceFormatKHR GetPreferredSurfaceFormat();
	VkPresentModeKHR   GetPreferredPresentMode();
	VkExtent2D		   GetPreferredExtent();

private:
	VkInstance Instance = VK_NULL_HANDLE;
	VkDevice   Device	= VK_NULL_HANDLE;

	VkSurfaceKHR Surface = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR		SurfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR>	PresentModes;

	VkSwapchainKHR			 VkSwapchain = VK_NULL_HANDLE;
	VkFormat				 Format		 = VK_FORMAT_UNDEFINED;
	VkExtent2D				 Extent		 = { 0, 0 };
	std::vector<VkImage>	 Images;
	std::vector<VkImageView> ImageViews;

	VulkanCommandQueue* PresentQueue = nullptr;
	uint32_t			CurrentImageIndex;
};
