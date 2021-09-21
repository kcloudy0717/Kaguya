#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace VulkanAPI
{
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = nullptr;
}

static constexpr const char* ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };

static constexpr const char* DebugInstanceExtensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
														   VK_KHR_SURFACE_EXTENSION_NAME,
														   VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
														   VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };

static constexpr const char* InstanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME,
													  VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

static constexpr const char* LogicalExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
													 VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
													 VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
													 VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME };

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
	default:
		break;
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
	VkDebugUtilsMessageSeverityFlagBitsEXT		MessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT				MessageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*										pUserData)
{
	switch (MessageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
	{
		LOG_INFO(
			"{}: {}\n{}\n",
			GetMessageSeverity(MessageSeverity),
			GetMessageType(MessageType),
			pCallbackData->pMessage);
	}
	break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
	{
		LOG_WARN(
			"{}: {}\n{}\n",
			GetMessageSeverity(MessageSeverity),
			GetMessageType(MessageType),
			pCallbackData->pMessage);
	}
	break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	{
		LOG_ERROR(
			"{}: {}\n{}\n",
			GetMessageSeverity(MessageSeverity),
			GetMessageType(MessageType),
			pCallbackData->pMessage);
	}
	break;

	default:
		break;
	}
	return VK_FALSE;
}

VulkanDevice::VulkanDevice()
	: GraphicsQueue(this)
	, AsyncComputeQueue(this)
	, CopyQueue(this)
{
}

VulkanDevice::~VulkanDevice()
{
	SamplerDescriptorHeap.Destroy();
	ResourceDescriptorHeap.Destroy();
	CopyQueue.Destroy();
	AsyncComputeQueue.Destroy();
	GraphicsQueue.Destroy();

	vmaDestroyAllocator(Allocator);

	vkDestroyDevice(VkDevice, nullptr);

	if (DebugUtilsMessenger)
	{
		auto vkDestroyDebugUtilsMessengerEXT =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT)
		{
			vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
		}
	}
	vkDestroyInstance(Instance, nullptr);
}

void VulkanDevice::Initialize(const DeviceOptions& Options)
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
	ApplicationInfo.pApplicationName   = "Vulkan Engine";
	ApplicationInfo.applicationVersion = 0;
	ApplicationInfo.pEngineName		   = nullptr;
	ApplicationInfo.engineVersion	   = 0;
	ApplicationInfo.apiVersion		   = VK_API_VERSION_1_2;

	auto InstanceCreateInfo				= VkStruct<VkInstanceCreateInfo>();
	InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;

	if (Options.EnableDebugLayer)
	{
		InstanceCreateInfo.pNext			   = &DebugUtilsMessengerCreateInfo;
		InstanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(std::size(ValidationLayers));
		InstanceCreateInfo.ppEnabledLayerNames = ValidationLayers;
	}

	uint32_t NumExtensions = Options.EnableDebugLayer ? static_cast<uint32_t>(std::size(DebugInstanceExtensions))
													  : static_cast<uint32_t>(std::size(InstanceExtensions));
	auto	 Extensions	   = Options.EnableDebugLayer ? DebugInstanceExtensions : InstanceExtensions;

	InstanceCreateInfo.enabledExtensionCount   = NumExtensions;
	InstanceCreateInfo.ppEnabledExtensionNames = Extensions;

	VERIFY_VULKAN_API(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance));

	if (Options.EnableDebugLayer)
	{
		VERIFY_VULKAN_API(
			CreateDebugUtilsMessengerEXT(Instance, &DebugUtilsMessengerCreateInfo, nullptr, &DebugUtilsMessenger));
	}

	uint32_t					  NumPhysicalDevices = 0;
	std::vector<VkPhysicalDevice> PhysicalDevices;
	VERIFY_VULKAN_API(vkEnumeratePhysicalDevices(Instance, &NumPhysicalDevices, nullptr));
	PhysicalDevices.resize(NumPhysicalDevices);
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
	uint32_t QueueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, nullptr);
	QueueFamilyProperties.resize(QueueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueFamilyProperties.data());

	for (uint32_t i = 0; i < QueueFamilyPropertyCount; ++i)
	{
		VkQueueFlags QueueFlags = QueueFamilyProperties[i].queueFlags;

		if (QueueFlags & VK_QUEUE_GRAPHICS_BIT && !GraphicsFamily.has_value())
		{
			GraphicsFamily = i;
		}
		else if (QueueFlags & VK_QUEUE_COMPUTE_BIT && !ComputeFamily.has_value())
		{
			ComputeFamily = i;
		}
		else if (QueueFlags & VK_QUEUE_TRANSFER_BIT && !CopyFamily.has_value())
		{
			CopyFamily = i;
		}
	}
}

void VulkanDevice::InitializeDevice()
{
	constexpr float QueuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> DeviceQueueCreateInfos;
	std::set<uint32_t> QueueFamilyIndexSet = { GraphicsFamily.value(), ComputeFamily.value(), CopyFamily.value() };

	for (uint32_t QueueFamilyIndex : QueueFamilyIndexSet)
	{
		auto& DeviceQueueCreateInfo = DeviceQueueCreateInfos.emplace_back(VkStruct<VkDeviceQueueCreateInfo>());
		DeviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
		DeviceQueueCreateInfo.queueCount	   = 1;
		DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;
	}

	auto PhysicalDeviceVulkan12Features							   = VkStruct<VkPhysicalDeviceVulkan12Features>();
	PhysicalDeviceVulkan12Features.descriptorIndexing			   = VK_TRUE;
	PhysicalDeviceVulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
	PhysicalDeviceVulkan12Features.runtimeDescriptorArray		   = VK_TRUE;
	PhysicalDeviceVulkan12Features.timelineSemaphore			   = VK_TRUE;

	auto DeviceCreateInfo					 = VkStruct<VkDeviceCreateInfo>();
	DeviceCreateInfo.pNext					 = &PhysicalDeviceVulkan12Features;
	DeviceCreateInfo.queueCreateInfoCount	 = static_cast<uint32_t>(DeviceQueueCreateInfos.size());
	DeviceCreateInfo.pQueueCreateInfos		 = DeviceQueueCreateInfos.data();
	DeviceCreateInfo.enabledExtensionCount	 = static_cast<uint32_t>(std::size(LogicalExtensions));
	DeviceCreateInfo.ppEnabledExtensionNames = LogicalExtensions;
	DeviceCreateInfo.pEnabledFeatures		 = &Features;

	VERIFY_VULKAN_API(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &VkDevice));

	InitializeVulkanAPI();

	VmaAllocatorCreateInfo AllocatorCreateInfo = {};
	AllocatorCreateInfo.physicalDevice		   = PhysicalDevice;
	AllocatorCreateInfo.device				   = VkDevice;
	AllocatorCreateInfo.instance			   = Instance;
	VERIFY_VULKAN_API(vmaCreateAllocator(&AllocatorCreateInfo, &Allocator));

	GraphicsQueue.Initialize(GraphicsFamily.value());
	AsyncComputeQueue.Initialize(ComputeFamily.value());
	CopyQueue.Initialize(CopyFamily.value());

	ResourceDescriptorHeapDesc Desc = {};
	Desc.NumTextureDescriptors		= 4096;
	Desc.NumRWTextureDescriptors	= 4096;

	ResourceDescriptorHeap = VulkanResourceDescriptorHeap(this, Desc);
	SamplerDescriptorHeap  = VulkanSamplerDescriptorHeap(this, 2048);
}

VulkanCommandQueue* VulkanDevice::InitializePresentQueue(VkSurfaceKHR Surface)
{
	VkBool32 PresentSupport = VK_FALSE;
	uint32	 FamilyIndex	= GraphicsQueue.GetQueueFamilyIndex();
	VERIFY_VULKAN_API(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &PresentSupport));
	assert(PresentSupport && "Queue does not support present");
	return &GraphicsQueue;
}

bool VulkanDevice::QueryValidationLayerSupport()
{
	uint32_t PropertyCount = 0;
	VERIFY_VULKAN_API(vkEnumerateInstanceLayerProperties(&PropertyCount, nullptr));
	std::vector<VkLayerProperties> Properties(PropertyCount);
	VERIFY_VULKAN_API(vkEnumerateInstanceLayerProperties(&PropertyCount, Properties.data()));

	return std::ranges::all_of(
		std::begin(ValidationLayers),
		std::end(ValidationLayers),
		[&](const char* Layer)
		{
			for (const auto& Property : Properties)
			{
				if (strcmp(Layer, Property.layerName) == 0)
				{
					return true;
				}
			}
			return false;
		});
}

bool VulkanDevice::IsPhysicalDeviceSuitable(VkPhysicalDevice PhysicalDevice)
{
	vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &Features);
	auto vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(
		vkGetInstanceProcAddr(Instance, "vkGetPhysicalDeviceProperties2KHR"));
	VkPhysicalDeviceProperties2KHR DeviceProperties2KHR{};
	PushDescriptorProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
	DeviceProperties2KHR.sType	   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
	DeviceProperties2KHR.pNext	   = &PushDescriptorProperties;
	vkGetPhysicalDeviceProperties2KHR(PhysicalDevice, &DeviceProperties2KHR);

	// http://roar11.com/2019/06/vulkan-textures-unbound/
	// Bindless
	auto PhysicalDeviceDescriptorIndexingFeatures = VkStruct<VkPhysicalDeviceDescriptorIndexingFeatures>();

	Features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	Features2.pNext = &PhysicalDeviceDescriptorIndexingFeatures;
	vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features2);

	bool BindlessSupport = PhysicalDeviceDescriptorIndexingFeatures.descriptorBindingPartiallyBound &&
						   PhysicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray;

	return Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		   CheckDeviceExtensionSupport(PhysicalDevice) && BindlessSupport;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) const
{
	uint32_t PropertyCount = 0;
	VERIFY_VULKAN_API(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, nullptr));
	std::vector<VkExtensionProperties> Properties(PropertyCount);
	VERIFY_VULKAN_API(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, Properties.data()));

	std::set<std::string> RequiredExtensions(std::begin(LogicalExtensions), std::end(LogicalExtensions));

	for (const auto& [ExtensionName, SpecVersion] : Properties)
	{
		RequiredExtensions.erase(ExtensionName);
	}

	return RequiredExtensions.empty();
}

void VulkanDevice::InitializeVulkanAPI()
{
#define VULKAN_QUERY_API(API)                                                                                          \
	VulkanAPI::API = reinterpret_cast<decltype(VulkanAPI::API)>(vkGetDeviceProcAddr(VkDevice, #API));                  \
	assert(VulkanAPI::API)

	VULKAN_QUERY_API(vkCmdPushDescriptorSetKHR);

#undef VULKAN_QUERY_API
}

RefPtr<IRHIRenderPass> VulkanDevice::CreateRenderPass(const RenderPassDesc& Desc)
{
	return RefPtr<IRHIRenderPass>::Create(new VulkanRenderPass(this, Desc));
}

RefPtr<IRHIRenderTarget> VulkanDevice::CreateRenderTarget(const RenderTargetDesc& Desc)
{
	return RefPtr<IRHIRenderTarget>::Create(new VulkanRenderTarget(this, Desc));
}

RefPtr<IRHIRootSignature> VulkanDevice::CreateRootSignature(const RootSignatureDesc& Desc)
{
	return RefPtr<IRHIRootSignature>::Create(new VulkanRootSignature(this, Desc));
}

RefPtr<IRHIPipelineState> VulkanDevice::CreatePipelineState(const PipelineStateStreamDesc& Desc)
{
	return RefPtr<IRHIPipelineState>::Create(new VulkanPipelineState(this, Desc));
}

DescriptorHandle VulkanDevice::AllocateShaderResourceView()
{
	UINT Index = UINT_MAX;
	ResourceDescriptorHeap.Allocate(1, Index);

	DescriptorHandle Handle;
	Handle.Version = 0;
	Handle.Api	   = 0;
	Handle.Index   = Index;
	return Handle;
}

DescriptorHandle VulkanDevice::AllocateSampler()
{
	UINT Index = UINT_MAX;
	SamplerDescriptorHeap.Allocate(Index);

	DescriptorHandle Handle;
	Handle.Version = 0;
	Handle.Api	   = 0;
	Handle.Index   = Index;
	return Handle;
}

void VulkanDevice::ReleaseShaderResourceView(DescriptorHandle Handle)
{
	if (Handle.IsValid())
	{
		ResourceDescriptorHeap.Release(Handle.Api, Handle.Index);
	}
}

void VulkanDevice::ReleaseSampler(DescriptorHandle Handle)
{
	if (Handle.IsValid())
	{
		SamplerDescriptorHeap.Release(Handle.Index);
	}
}

void VulkanDevice::CreateShaderResourceView(
	IRHIResource*				  Resource,
	const ShaderResourceViewDesc& Desc,
	DescriptorHandle			  DestHandle)
{
	auto ApiTexture = Resource->As<VulkanTexture>();

	uint32_t MostDetailedMip = Desc.Texture2D.MostDetailedMip.value_or(0);
	uint32_t MipLevels		 = Desc.Texture2D.MipLevels.value_or(ApiTexture->Desc.mipLevels);

	auto ImageViewCreateInfo							= VkStruct<VkImageViewCreateInfo>();
	ImageViewCreateInfo.viewType						= VK_IMAGE_VIEW_TYPE_2D;
	ImageViewCreateInfo.image							= ApiTexture->GetApiHandle();
	ImageViewCreateInfo.format							= ApiTexture->Desc.format;
	ImageViewCreateInfo.subresourceRange.aspectMask		= InferImageAspectFlags(ApiTexture->Desc.format);
	ImageViewCreateInfo.subresourceRange.baseMipLevel	= MostDetailedMip;
	ImageViewCreateInfo.subresourceRange.levelCount		= MipLevels;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.layerCount		= ApiTexture->Desc.arrayLayers;

	VkImageView ImageView = VK_NULL_HANDLE;
	UINT64		Hash	  = CityHash64((char*)&ImageViewCreateInfo.subresourceRange, sizeof(VkImageSubresourceRange));
	if (auto iter = ApiTexture->ImageViewTable.find(Hash); iter != ApiTexture->ImageViewTable.end())
	{
		ImageView = iter->second;
	}
	else
	{
		VERIFY_VULKAN_API(vkCreateImageView(VkDevice, &ImageViewCreateInfo, nullptr, &ImageView));
		ApiTexture->ImageViewTable[Hash] = ImageView;
	}

	VkDescriptorImageInfo DescriptorImageInfo = {};
	DescriptorImageInfo.sampler				  = nullptr;
	DescriptorImageInfo.imageView			  = ImageView;
	DescriptorImageInfo.imageLayout			  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	auto WriteDescriptorSet			   = VkStruct<VkWriteDescriptorSet>();
	WriteDescriptorSet.dstSet		   = ResourceDescriptorHeap.DescriptorSet;
	WriteDescriptorSet.dstBinding	   = DestHandle.Api; // Texture
	WriteDescriptorSet.dstArrayElement = DestHandle.Index;
	WriteDescriptorSet.descriptorCount = 1;
	WriteDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	WriteDescriptorSet.pImageInfo	   = &DescriptorImageInfo;

	vkUpdateDescriptorSets(VkDevice, 1, &WriteDescriptorSet, 0, nullptr);
}

void VulkanDevice::CreateSampler(const SamplerDesc& Desc, DescriptorHandle DestHandle)
{
	auto SamplerCreateInfo			   = VkStruct<VkSamplerCreateInfo>();
	SamplerCreateInfo.flags			   = 0;
	SamplerCreateInfo.magFilter		   = ToVkFilter(Desc.Filter);
	SamplerCreateInfo.minFilter		   = ToVkFilter(Desc.Filter);
	SamplerCreateInfo.mipmapMode	   = ToVkSamplerMipmapMode(Desc.Filter);
	SamplerCreateInfo.addressModeU	   = ToVkSamplerAddressMode(Desc.AddressU);
	SamplerCreateInfo.addressModeV	   = ToVkSamplerAddressMode(Desc.AddressV);
	SamplerCreateInfo.addressModeW	   = ToVkSamplerAddressMode(Desc.AddressW);
	SamplerCreateInfo.mipLodBias	   = Desc.MipLODBias;
	SamplerCreateInfo.anisotropyEnable = Desc.MaxAnisotropy > 1;
	SamplerCreateInfo.maxAnisotropy	   = static_cast<float>(Desc.MaxAnisotropy);
	SamplerCreateInfo.compareEnable	   = Desc.ComparisonFunc != ComparisonFunc::Never;
	SamplerCreateInfo.compareOp		   = ToVkCompareOp(Desc.ComparisonFunc);
	SamplerCreateInfo.minLod		   = Desc.MinLOD;
	SamplerCreateInfo.maxLod		   = Desc.MaxLOD;
	SamplerCreateInfo.borderColor =
		Desc.BorderColor == 0 ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK : VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VkSampler Sampler = nullptr;
	UINT64	  Hash	  = CityHash64(reinterpret_cast<char*>(&SamplerCreateInfo), sizeof(VkSamplerCreateInfo));
	if (auto iter = SamplerDescriptorHeap.SamplerTable.find(Hash); iter != SamplerDescriptorHeap.SamplerTable.end())
	{
		Sampler = iter->second;
	}
	else
	{
		VERIFY_VULKAN_API(vkCreateSampler(VkDevice, &SamplerCreateInfo, nullptr, &Sampler));
		SamplerDescriptorHeap.SamplerTable[Hash] = Sampler;
	}

	VkDescriptorImageInfo DescriptorImageInfo = {};
	DescriptorImageInfo.sampler				  = Sampler;
	DescriptorImageInfo.imageView			  = nullptr;
	DescriptorImageInfo.imageLayout			  = VK_IMAGE_LAYOUT_UNDEFINED;

	auto WriteDescriptorSet			   = VkStruct<VkWriteDescriptorSet>();
	WriteDescriptorSet.dstSet		   = SamplerDescriptorHeap.DescriptorSet;
	WriteDescriptorSet.dstBinding	   = DestHandle.Api;
	WriteDescriptorSet.dstArrayElement = DestHandle.Index;
	WriteDescriptorSet.descriptorCount = 1;
	WriteDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
	WriteDescriptorSet.pImageInfo	   = &DescriptorImageInfo;

	vkUpdateDescriptorSets(VkDevice, 1, &WriteDescriptorSet, 0, nullptr);
}

RefPtr<IRHIBuffer> VulkanDevice::CreateBuffer(const RHIBufferDesc& Desc)
{
	auto					BufferCreateInfo	 = VkStruct<VkBufferCreateInfo>();
	VmaAllocationCreateInfo AllocationCreateInfo = {};

	BufferCreateInfo.size  = Desc.SizeInBytes;
	BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	switch (Desc.HeapType)
	{
	case ERHIHeapType::DeviceLocal:
		AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		break;

	case ERHIHeapType::Upload:
		AllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		break;

	case ERHIHeapType::Readback:
		AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
		break;

	default:
		break;
	}

	if (Desc.Flags & ERHIBufferFlags::RHIBufferFlag_ConstantBuffer)
	{
		BufferCreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		AllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	}
	if (Desc.Flags & ERHIBufferFlags::RHIBufferFlag_VertexBuffer)
	{
		BufferCreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (Desc.Flags & ERHIBufferFlags::RHIBufferFlag_IndexBuffer)
	{
		BufferCreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	return RefPtr<IRHIBuffer>::Create(new VulkanBuffer(this, BufferCreateInfo, AllocationCreateInfo));
}

RefPtr<IRHITexture> VulkanDevice::CreateTexture(const RHITextureDesc& Desc)
{
	auto					ImageCreateInfo		 = VkStruct<VkImageCreateInfo>();
	VmaAllocationCreateInfo AllocationCreateInfo = {};

	ImageCreateInfo.imageType	= ToVkImageType(Desc.Type);
	ImageCreateInfo.format		= ToVkFormat(Desc.Format);
	ImageCreateInfo.extent		= { .width = Desc.Width, .height = Desc.Height, .depth = Desc.DepthOrArraySize };
	ImageCreateInfo.mipLevels	= Desc.MipLevels;
	ImageCreateInfo.arrayLayers = Desc.DepthOrArraySize;
	ImageCreateInfo.samples		= VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage =
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	AllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	if (Desc.Flags & ERHITextureFlags::RHITextureFlag_AllowRenderTarget)
	{
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (Desc.Flags & ERHITextureFlags::RHITextureFlag_AllowDepthStencil)
	{
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (Desc.Flags & ERHITextureFlags::RHITextureFlag_AllowUnorderedAccess)
	{
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	return RefPtr<IRHITexture>::Create(new VulkanTexture(this, ImageCreateInfo, AllocationCreateInfo));
}
