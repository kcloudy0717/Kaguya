#include "Adapter.h"
#include <iostream>

static constexpr const char* ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };

static constexpr const char* LogicalExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

static const char* GetMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity)
{
	switch (MessageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "[Verbose]";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "[Info]";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "[Warning]";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "[Error]";
	}
	return "<unknown>";
}

static const char* GetMessageType(VkDebugUtilsMessageTypeFlagsEXT MessageType)
{
	if (MessageType == 7)
	{
		return "[General | Validation | Performance]";
	}
	if (MessageType == 6)
	{
		return "[Validation | Performance]";
	}
	if (MessageType == 5)
	{
		return "[General | Performance]";
	}
	if (MessageType == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/)
	{
		return "[Performance]";
	}
	if (MessageType == 3)
	{
		return "[General | Validation]";
	}
	if (MessageType == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/)
	{
		return "[Validation]";
	}
	if (MessageType == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/)
	{
		return "[General]";
	}
	return "<unknown>";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL MessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT				messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*										pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
	{
		LOG_INFO(
			"{}: {}\n{}\n",
			GetMessageSeverity(messageSeverity),
			GetMessageType(messageType),
			pCallbackData->pMessage);
	}
	break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
	{
		LOG_WARN(
			"{}: {}\n{}\n",
			GetMessageSeverity(messageSeverity),
			GetMessageType(messageType),
			pCallbackData->pMessage);
	}
	break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	{
		LOG_ERROR(
			"{}: {}\n{}\n",
			GetMessageSeverity(messageSeverity),
			GetMessageType(messageType),
			pCallbackData->pMessage);
	}
	break;
	}
	return VK_FALSE;
}

Adapter::Adapter()
	: Device(this)
{
}

Adapter::~Adapter()
{
	Device.Destroy();

	vkDestroyDevice(VkDevice, nullptr);

	auto vkDestroyDebugUtilsMessengerEXT =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT)
	{
		vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
	}
	vkDestroyInstance(Instance, nullptr);
}

void Adapter::Initialize(const DeviceOptions& Options)
{
	if (Options.EnableDebugLayer && !QueryValidationLayerSupport())
	{
		throw std::runtime_error("Validation Layer Unsupported");
	}

	auto DebugUtilsMessengerCreateInfo			  = VkStruct<VkDebugUtilsMessengerCreateInfoEXT>();
	DebugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
													VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
													VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	DebugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
												VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
												VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	DebugUtilsMessengerCreateInfo.pfnUserCallback = MessageCallback;

	auto ApplicationInfo			   = VkStruct<VkApplicationInfo>();
	ApplicationInfo.pNext			   = nullptr;
	ApplicationInfo.pApplicationName   = "Vulkan Engine";
	ApplicationInfo.applicationVersion = 0;
	ApplicationInfo.pEngineName		   = nullptr;
	ApplicationInfo.engineVersion	   = 0;
	ApplicationInfo.apiVersion		   = VK_API_VERSION_1_0;

	auto InstanceCreateInfo				= VkStruct<VkInstanceCreateInfo>();
	InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;

	if (Options.EnableDebugLayer)
	{
		InstanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(std::size(ValidationLayers));
		InstanceCreateInfo.ppEnabledLayerNames = ValidationLayers;
	}

	InstanceCreateInfo.pNext = &DebugUtilsMessengerCreateInfo;

	auto Extensions							   = GetExtensions(Options.EnableDebugLayer);
	InstanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(Extensions.size());
	InstanceCreateInfo.ppEnabledExtensionNames = Extensions.data();

	VERIFY_VULKAN_API(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance));

	VERIFY_VULKAN_API(
		CreateDebugUtilsMessengerEXT(Instance, &DebugUtilsMessengerCreateInfo, nullptr, &DebugUtilsMessenger));

	uint32_t NumPhysicalDevices = 0;
	VERIFY_VULKAN_API(vkEnumeratePhysicalDevices(Instance, &NumPhysicalDevices, nullptr));

	std::vector<VkPhysicalDevice> PhysicalDevices(NumPhysicalDevices);
	VERIFY_VULKAN_API(vkEnumeratePhysicalDevices(Instance, &NumPhysicalDevices, PhysicalDevices.data()));

	for (const auto& PhysicalDevice : PhysicalDevices)
	{
		if (IsPhysicalDeviceSuitable(PhysicalDevice))
		{
			this->PhysicalDevice = PhysicalDevice;
			break;
		}
	}

	// Queue Family
	{
		uint32_t QueueFamilyPropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, nullptr);

		QueueFamilyProperties.resize(QueueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(
			PhysicalDevice,
			&QueueFamilyPropertyCount,
			QueueFamilyProperties.data());
	}
}

void Adapter::InitializeDevice()
{
	constexpr float QueuePriority = 1.0f;

	QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> DeviceQueueCreateInfos;
	std::set<uint32_t>					 QueueFamilyIndexSet = { Indices.GraphicsFamily.value() };

	for (uint32_t QueueFamilyIndex : QueueFamilyIndexSet)
	{
		auto& DeviceQueueCreateInfo = DeviceQueueCreateInfos.emplace_back(VkStruct<VkDeviceQueueCreateInfo>());
		DeviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
		DeviceQueueCreateInfo.queueCount	   = 1;
		DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;
	}

	auto DeviceCreateInfo					 = VkStruct<VkDeviceCreateInfo>();
	DeviceCreateInfo.queueCreateInfoCount	 = static_cast<uint32_t>(DeviceQueueCreateInfos.size());
	DeviceCreateInfo.pQueueCreateInfos		 = DeviceQueueCreateInfos.data();
	DeviceCreateInfo.enabledExtensionCount	 = static_cast<uint32_t>(std::size(LogicalExtensions));
	DeviceCreateInfo.ppEnabledExtensionNames = LogicalExtensions;
	DeviceCreateInfo.pEnabledFeatures		 = &PhysicalDeviceFeatures;

	VERIFY_VULKAN_API(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &VkDevice));

	Device.Initialize(Indices.GraphicsFamily.value());
}

CommandQueue* Adapter::InitializePresentQueue(VkSurfaceKHR Surface)
{
	VkBool32	 PresentSupport = VK_FALSE;
	const uint32 FamilyIndex	= Device.GetGraphicsQueue()->GetQueueFamilyIndex();
	vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &PresentSupport);
	assert(PresentSupport && "Queue does not support present");
	return Device.GetGraphicsQueue();
}

bool Adapter::QueryValidationLayerSupport()
{
	uint32_t PropertyCount;
	vkEnumerateInstanceLayerProperties(&PropertyCount, nullptr);

	std::vector<VkLayerProperties> Properties(PropertyCount);
	vkEnumerateInstanceLayerProperties(&PropertyCount, Properties.data());

	for (const char* Layer : ValidationLayers)
	{
		bool LayerFound = false;

		for (const auto& Property : Properties)
		{
			if (strcmp(Layer, Property.layerName) == 0)
			{
				LayerFound = true;
				break;
			}
		}

		if (!LayerFound)
		{
			return false;
		}
	}

	return true;
}

bool Adapter::IsPhysicalDeviceSuitable(VkPhysicalDevice PhysicalDevice)
{
	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceFeatures);

	return PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		   CheckDeviceExtensionSupport(PhysicalDevice);
}

bool Adapter::CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice)
{
	uint32_t PropertyCount;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, nullptr);

	std::vector<VkExtensionProperties> Properties(PropertyCount);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, Properties.data());

	std::set<std::string> RequiredExtensions(std::begin(LogicalExtensions), std::end(LogicalExtensions));

	for (const auto& Property : Properties)
	{
		RequiredExtensions.erase(Property.extensionName);
	}

	return RequiredExtensions.empty();
}
