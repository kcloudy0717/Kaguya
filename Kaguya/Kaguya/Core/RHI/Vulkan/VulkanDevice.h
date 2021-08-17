#pragma once
#include <Core/RHI/RHICommon.h>
#include "VulkanCommon.h"
#include "VulkanCommandQueue.h"
#include "VulkanResource.h"

class VulkanDevice
{
public:
	VulkanDevice();
	~VulkanDevice();

	void Initialize(const DeviceOptions& Options);
	void InitializeDevice();

	VulkanCommandQueue* InitializePresentQueue(VkSurfaceKHR Surface);

	[[nodiscard]] auto GetVkInstance() const noexcept -> VkInstance { return Instance; }
	[[nodiscard]] auto GetVkPhysicalDevice() const noexcept -> VkPhysicalDevice { return PhysicalDevice; }
	[[nodiscard]] auto GetVkDevice() const noexcept -> VkDevice { return VkDevice; }
	[[nodiscard]] auto GetVkAllocator() const noexcept -> VmaAllocator { return Allocator; }
	[[nodiscard]] auto GetGraphicsQueue() noexcept -> VulkanCommandQueue& { return GraphicsQueue; }

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

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice PhysicalDevice)
	{
		QueueFamilyIndices Indices;

		int i = 0;
		for (const auto& Properties : QueueFamilyProperties)
		{
			if (Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				Indices.GraphicsFamily = i;
			}

			if (Indices.IsValid())
			{
				break;
			}

			i++;
		}

		return Indices;
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice);

private:
	VkInstance				 Instance			 = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT DebugUtilsMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice		   PhysicalDevice			= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties PhysicalDeviceProperties = {};
	VkPhysicalDeviceFeatures   PhysicalDeviceFeatures	= {};

	std::vector<VkQueueFamilyProperties> QueueFamilyProperties;

	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR>	PresentModes;

	VkDevice	 VkDevice;
	VmaAllocator Allocator;

	VulkanCommandQueue GraphicsQueue;
};
