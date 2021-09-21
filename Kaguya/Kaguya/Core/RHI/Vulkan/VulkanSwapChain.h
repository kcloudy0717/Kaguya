#pragma once
#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanSwapChain
{
public:
	VulkanSwapChain() noexcept = default;
	~VulkanSwapChain();

	void Initialize(HWND hWnd, VulkanDevice* Device);

	[[nodiscard]] auto GetFormat() const noexcept -> VkFormat { return Format; }
	[[nodiscard]] auto GetExtent() const noexcept -> VkExtent2D { return Extent; }
	[[nodiscard]] auto GetWidth() const noexcept -> uint32_t { return Extent.width; }
	[[nodiscard]] auto GetHeight() const noexcept -> uint32_t { return Extent.height; }
	[[nodiscard]] auto GetImageCount() const noexcept -> uint32_t { return static_cast<uint32_t>(Images.size()); }
	[[nodiscard]] auto GetImageView(size_t i) const noexcept -> VkImageView;
	[[nodiscard]] auto GetCurrentBackbuffer() -> IRHITexture* { return Backbuffers[CurrentImageIndex].get(); }
	[[nodiscard]] auto GetBackbuffer(size_t i) -> IRHITexture* { return Backbuffers[i].get(); }

	[[nodiscard]] auto GetRHIFormat() const noexcept -> ERHIFormat { return ERHIFormat::SBGRA8_UNORM; }

	uint32_t AcquireNextImage();

	void Present();

private:
	[[nodiscard]] auto GetPreferredSurfaceFormat() const noexcept -> VkSurfaceFormatKHR;
	[[nodiscard]] auto GetPreferredPresentMode() const noexcept -> VkPresentModeKHR;
	[[nodiscard]] auto GetPreferredExtent() const noexcept -> VkExtent2D;

private:
	VkInstance Instance = VK_NULL_HANDLE;
	VkDevice   Device	= VK_NULL_HANDLE;

	VkSurfaceKHR Surface = VK_NULL_HANDLE;

	VkSurfaceCapabilitiesKHR		SurfaceCapabilities = {};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR>	PresentModes;

	VkSwapchainKHR								VkSwapchain = VK_NULL_HANDLE;
	VkFormat									Format		= VK_FORMAT_UNDEFINED;
	VkExtent2D									Extent		= { 0, 0 };
	std::vector<VkImage>						Images;
	std::vector<VkImageView>					ImageViews;
	std::vector<std::unique_ptr<VulkanTexture>> Backbuffers;

	VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
	VkSemaphore RenderSemaphore	 = VK_NULL_HANDLE;

	VulkanCommandQueue* PresentQueue = nullptr;
	uint32_t			CurrentImageIndex;
};
