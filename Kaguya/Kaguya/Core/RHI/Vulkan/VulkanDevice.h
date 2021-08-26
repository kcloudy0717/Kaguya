#pragma once
#include <Core/RHI/RHICommon.h>
#include <Core/RHI/RHIDevice.h>
#include <Core/RHI/RHIResourceManager.h>
#include "VulkanCommon.h"
#include "VulkanCommandQueue.h"
#include "VulkanResource.h"
#include "VulkanDescriptorHeap.h"

class VulkanRenderPass final : public IRHIRenderPass
{
public:
	VulkanRenderPass() noexcept = default;
	VulkanRenderPass(VulkanDevice* Parent, const RenderPassDesc& Desc);
	~VulkanRenderPass();

	auto GetParentDevice() const noexcept -> VulkanDevice*;

	VulkanRenderPass(VulkanRenderPass&& VulkanRenderPass)
		: IRHIRenderPass(std::exchange(VulkanRenderPass.Parent, {}))
		, RenderPass(std::exchange(VulkanRenderPass.RenderPass, {}))
	{
	}
	VulkanRenderPass& operator=(VulkanRenderPass&& VulkanRenderPass)
	{
		if (this != &VulkanRenderPass)
		{
			Parent	   = std::exchange(VulkanRenderPass.Parent, {});
			RenderPass = std::exchange(VulkanRenderPass.RenderPass, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanRenderPass);

	[[nodiscard]] VkRenderPass GetApiHandle() const noexcept { return RenderPass; }

private:
	VkRenderPass		  RenderPass		  = VK_NULL_HANDLE;
	VkRenderPassBeginInfo RenderPassBeginInfo = {};
};

class VulkanDevice final : public IRHIDevice
{
public:
	VulkanDevice();
	~VulkanDevice() override;

	void Initialize(const DeviceOptions& Options);
	void InitializeDevice();

	VulkanCommandQueue* InitializePresentQueue(VkSurfaceKHR Surface);

	[[nodiscard]] auto GetVkInstance() const noexcept -> VkInstance { return Instance; }
	[[nodiscard]] auto GetVkPhysicalDevice() const noexcept -> VkPhysicalDevice { return PhysicalDevice; }
	[[nodiscard]] auto GetVkDevice() const noexcept -> VkDevice { return VkDevice; }
	[[nodiscard]] auto GetVkAllocator() const noexcept -> VmaAllocator { return Allocator; }
	[[nodiscard]] auto GetGraphicsQueue() noexcept -> VulkanCommandQueue& { return GraphicsQueue; }
	[[nodiscard]] auto GetCopyQueue() noexcept -> VulkanCommandQueue& { return CopyQueue; }

	// clang-format off
	template<typename T>	[[nodiscard]] auto GetResourcePool() -> TPool<T>&;
	template<>				[[nodiscard]] auto GetResourcePool() -> TPool<VulkanRenderPass>& { return RenderPassPool; }
	template<>				[[nodiscard]] auto GetResourcePool() -> TPool<VulkanBuffer>& { return BufferPool; }
	template<>				[[nodiscard]] auto GetResourcePool() -> TPool<VulkanTexture>& { return TexturePool; }
	// clang-format on

	[[nodiscard]] RefPtr<IRHIRenderPass> CreateRenderPass(const RenderPassDesc& Desc) override;

	[[nodiscard]] RefPtr<IRHIBuffer>  CreateBuffer(const RHIBufferDesc& Desc) override;
	[[nodiscard]] RefPtr<IRHITexture> CreateTexture(const RHITextureDesc& Desc) override;

private:
	std::vector<const char*> GetExtensions(bool EnableDebugLayer)
	{
		std::vector<const char*> Extensions;

		if (EnableDebugLayer)
		{
			Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		Extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		Extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		return Extensions;
	}

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance								  instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks*			  pAllocator,
		VkDebugUtilsMessengerEXT*				  pDebugMessenger)
	{
		auto vkCreateDebugUtilsMessengerEXT =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		return vkCreateDebugUtilsMessengerEXT
				   ? vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pDebugMessenger)
				   : VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	bool QueryValidationLayerSupport();

	bool IsPhysicalDeviceSuitable(VkPhysicalDevice PhysicalDevice);

	QueueFamilyIndices FindQueueFamilies()
	{
		QueueFamilyIndices Indices;

		int i = 0;
		for (const auto& Properties : QueueFamilyProperties)
		{
			if (Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				Indices.GraphicsFamily = i;
			}
			if (Properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				Indices.CopyFamily = i;
			}

			if (Indices.IsValid())
			{
				break;
			}

			i++;
		}

		return Indices;
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) const;

private:
	VkInstance				 Instance			 = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT DebugUtilsMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice		   PhysicalDevice			= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties PhysicalDeviceProperties = {};
	VkPhysicalDeviceFeatures   PhysicalDeviceFeatures	= {};
	VkPhysicalDeviceFeatures2  PhysicalDeviceFeatures2	= {};

	std::vector<VkQueueFamilyProperties> QueueFamilyProperties;

	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR>	PresentModes;

	VkDevice	 VkDevice  = VK_NULL_HANDLE;
	VmaAllocator Allocator = VK_NULL_HANDLE;

	VulkanCommandQueue GraphicsQueue;
	VulkanCommandQueue CopyQueue;

	TPool<VulkanRenderPass> RenderPassPool;
	TPool<VulkanBuffer>		BufferPool;
	TPool<VulkanTexture>	TexturePool;
};
