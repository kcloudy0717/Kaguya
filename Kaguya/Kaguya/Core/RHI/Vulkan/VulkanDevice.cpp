#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

static constexpr const char* ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };

static constexpr const char* LogicalExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
													 VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };

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

RefPtr<IRHIRenderPass> VulkanDevice::CreateRenderPass(const RenderPassDesc& Desc)
{
	return RefPtr<IRHIRenderPass>::Create(RenderPassPool.Construct(this, Desc));
}

RefPtr<IRHIDescriptorTable> VulkanDevice::CreateDescriptorTable(const DescriptorTableDesc& Desc)
{
	return RefPtr<IRHIDescriptorTable>::Create(DescriptorTablePool.Construct(this, Desc));
}

RefPtr<IRHIRootSignature> VulkanDevice::CreateRootSignature(const RootSignatureDesc& Desc)
{
	return RefPtr<IRHIRootSignature>::Create(RootSignaturePool.Construct(this, Desc));
}

RefPtr<IRHIDescriptorPool> VulkanDevice::CreateDescriptorPool(const DescriptorPoolDesc& Desc)
{
	return RefPtr<IRHIDescriptorPool>::Create(DescriptorPoolAllocator.Construct(this, Desc));
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

	return RefPtr<IRHIBuffer>::Create(BufferPool.Construct(this, BufferCreateInfo, AllocationCreateInfo));
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

	return RefPtr<IRHITexture>::Create(TexturePool.Construct(this, ImageCreateInfo, AllocationCreateInfo));
}

VulkanDevice::VulkanDevice()
	: GraphicsQueue(this)
	, CopyQueue(this)
	, RenderPassPool(64)
	, DescriptorTablePool(64)
	, RootSignaturePool(64)
	, DescriptorPoolAllocator(64)
	, BufferPool(2048)
	, TexturePool(2048)
{
}

VulkanDevice::~VulkanDevice()
{
	TexturePool.Destroy();
	BufferPool.Destroy();
	CopyQueue.Destroy();
	GraphicsQueue.Destroy();

	vmaDestroyAllocator(Allocator);

	vkDestroyDevice(VkDevice, nullptr);

	auto vkDestroyDebugUtilsMessengerEXT =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT)
	{
		vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
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

void VulkanDevice::InitializeDevice()
{
	constexpr float QueuePriority = 1.0f;

	QueueFamilyIndices Indices = FindQueueFamilies();

	std::vector<VkDeviceQueueCreateInfo> DeviceQueueCreateInfos;
	std::set<uint32_t> QueueFamilyIndexSet = { Indices.GraphicsFamily.value(), Indices.CopyFamily.value() };

	for (uint32_t QueueFamilyIndex : QueueFamilyIndexSet)
	{
		auto& DeviceQueueCreateInfo = DeviceQueueCreateInfos.emplace_back(VkStruct<VkDeviceQueueCreateInfo>());
		DeviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
		DeviceQueueCreateInfo.queueCount	   = 1;
		DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;
	}

	// Bindless
	auto PhysicalDeviceDescriptorIndexingFeatures = VkStruct<VkPhysicalDeviceDescriptorIndexingFeatures>();
	PhysicalDeviceDescriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	PhysicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray			 = VK_TRUE;

	auto DeviceCreateInfo					 = VkStruct<VkDeviceCreateInfo>();
	DeviceCreateInfo.pNext					 = &PhysicalDeviceDescriptorIndexingFeatures;
	DeviceCreateInfo.queueCreateInfoCount	 = static_cast<uint32_t>(DeviceQueueCreateInfos.size());
	DeviceCreateInfo.pQueueCreateInfos		 = DeviceQueueCreateInfos.data();
	DeviceCreateInfo.enabledExtensionCount	 = static_cast<uint32_t>(std::size(LogicalExtensions));
	DeviceCreateInfo.ppEnabledExtensionNames = LogicalExtensions;
	DeviceCreateInfo.pEnabledFeatures		 = &PhysicalDeviceFeatures;

	VERIFY_VULKAN_API(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &VkDevice));

	VmaAllocatorCreateInfo AllocatorCreateInfo = {};
	AllocatorCreateInfo.physicalDevice		   = PhysicalDevice;
	AllocatorCreateInfo.device				   = VkDevice;
	AllocatorCreateInfo.instance			   = Instance;
	VERIFY_VULKAN_API(vmaCreateAllocator(&AllocatorCreateInfo, &Allocator));

	GraphicsQueue.Initialize(Indices.GraphicsFamily.value());
	CopyQueue.Initialize(Indices.CopyFamily.value());
}

VulkanCommandQueue* VulkanDevice::InitializePresentQueue(VkSurfaceKHR Surface)
{
	VkBool32	 PresentSupport = VK_FALSE;
	const uint32 FamilyIndex	= GraphicsQueue.GetQueueFamilyIndex();
	vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &PresentSupport);
	assert(PresentSupport && "Queue does not support present");
	return &GraphicsQueue;
}

bool VulkanDevice::QueryValidationLayerSupport()
{
	uint32_t PropertyCount;
	vkEnumerateInstanceLayerProperties(&PropertyCount, nullptr);

	std::vector<VkLayerProperties> Properties(PropertyCount);
	vkEnumerateInstanceLayerProperties(&PropertyCount, Properties.data());

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
	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceFeatures);

	// http://roar11.com/2019/06/vulkan-textures-unbound/
	// Bindless
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT PhysicalDeviceDescriptorIndexingFeatures = {};
	PhysicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	PhysicalDeviceDescriptorIndexingFeatures.pNext = nullptr;

	PhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	PhysicalDeviceFeatures2.pNext = &PhysicalDeviceDescriptorIndexingFeatures;
	vkGetPhysicalDeviceFeatures2(PhysicalDevice, &PhysicalDeviceFeatures2);

	bool BindlessSupport = PhysicalDeviceDescriptorIndexingFeatures.descriptorBindingPartiallyBound &&
						   PhysicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray;

	return PhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		   CheckDeviceExtensionSupport(PhysicalDevice) && BindlessSupport;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) const
{
	uint32_t PropertyCount;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, nullptr);

	std::vector<VkExtensionProperties> Properties(PropertyCount);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &PropertyCount, Properties.data());

	std::set<std::string> RequiredExtensions(std::begin(LogicalExtensions), std::end(LogicalExtensions));

	for (const auto& [ExtensionName, SpecVersion] : Properties)
	{
		RequiredExtensions.erase(ExtensionName);
	}

	return RequiredExtensions.empty();
}

VulkanRenderPass::VulkanRenderPass(VulkanDevice* Parent, const RenderPassDesc& Desc)
	: IRHIRenderPass(Parent)
{
	bool ValidDepthStencilAttachment = Desc.DepthStencil.IsValid();

	UINT					NumAttachments	   = 0;
	VkAttachmentDescription Attachments[8 + 1] = {};

	// the renderpass will use this color attachment.
	UINT					NumRenderTargets = Desc.NumRenderTargets;
	VkAttachmentDescription RenderTargets[8] = {};
	VkAttachmentDescription DepthStencil	 = {};

	for (UINT i = 0; i < NumRenderTargets; ++i)
	{
		const auto& RHIRenderTarget = Desc.RenderTargets[i];

		RenderTargets[i].format			= RHIRenderTarget.Format;
		RenderTargets[i].samples		= VK_SAMPLE_COUNT_1_BIT;
		RenderTargets[i].loadOp			= ToVkLoadOp(RHIRenderTarget.LoadOp);
		RenderTargets[i].storeOp		= ToVkStoreOp(RHIRenderTarget.StoreOp);
		RenderTargets[i].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		RenderTargets[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// we don't know or care about the starting layout of the attachment
		RenderTargets[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		/////////////////////////////////// TODO FIX THIS
		// after the renderpass ends, the image has to be on a layout ready for display
		RenderTargets[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		Attachments[NumAttachments++] = RenderTargets[i];
	}

	if (ValidDepthStencilAttachment)
	{
		DepthStencil.format			= Desc.DepthStencil.Format;
		DepthStencil.samples		= VK_SAMPLE_COUNT_1_BIT;
		DepthStencil.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthStencil.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
		DepthStencil.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthStencil.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthStencil.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		DepthStencil.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		Attachments[NumAttachments++] = DepthStencil;
	}

	VkAttachmentReference ReferenceRenderTargets = {};
	ReferenceRenderTargets.attachment =
		0; // attachment number will index into the pAttachments array in the parent renderpass itself
	ReferenceRenderTargets.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ReferenceDepthStencil = {};
	ReferenceDepthStencil.attachment			= NumAttachments - 1;
	ReferenceDepthStencil.layout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription SubpassDescription	   = {};
	SubpassDescription.pipelineBindPoint	   = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDescription.colorAttachmentCount	   = NumRenderTargets;
	SubpassDescription.pColorAttachments	   = &ReferenceRenderTargets;
	SubpassDescription.pDepthStencilAttachment = ValidDepthStencilAttachment ? &ReferenceDepthStencil : nullptr;

	auto RenderPassCreateInfo = VkStruct<VkRenderPassCreateInfo>();
	// connect the color attachment to the info
	RenderPassCreateInfo.attachmentCount = NumAttachments;
	RenderPassCreateInfo.pAttachments	 = Attachments;
	// connect the subpass to the info
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses	  = &SubpassDescription;

	VERIFY_VULKAN_API(
		vkCreateRenderPass(Parent->As<VulkanDevice>()->GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
	if (Parent && RenderPass)
	{
		vkDestroyRenderPass(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(RenderPass, {}), nullptr);
	}
}

VulkanDescriptorTable::VulkanDescriptorTable(VulkanDevice* Parent, const DescriptorTableDesc& Desc)
	: IRHIDescriptorTable(Parent)
{
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	std::vector<VkDescriptorBindingFlagsEXT>  BindingFlags;

	for (auto [Type, NumDescriptors, Binding] : Desc.Ranges)
	{
		VkDescriptorSetLayoutBinding& VkBinding = Bindings.emplace_back();
		VkBinding.binding						= Binding;
		VkBinding.descriptorType				= ToVkDescriptorType(Type);
		VkBinding.descriptorCount				= NumDescriptors;
		VkBinding.stageFlags					= VK_SHADER_STAGE_ALL;
		BindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
	}

	auto BindingFlagsInfo				 = VkStruct<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>();
	BindingFlagsInfo.bindingCount		 = static_cast<uint32_t>(BindingFlags.size());
	BindingFlagsInfo.pBindingFlags		 = BindingFlags.data();
	auto DescriptorSetLayoutDesc		 = VkStruct<VkDescriptorSetLayoutCreateInfo>();
	DescriptorSetLayoutDesc.pNext		 = &BindingFlagsInfo;
	DescriptorSetLayoutDesc.flags		 = 0;
	DescriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(Bindings.size());
	DescriptorSetLayoutDesc.pBindings	 = Bindings.data();
	VERIFY_VULKAN_API(vkCreateDescriptorSetLayout(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetLayoutDesc,
		nullptr,
		&DescriptorSetLayout));
}

VulkanDescriptorTable::~VulkanDescriptorTable()
{
	if (Parent && DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(
			Parent->As<VulkanDevice>()->GetVkDevice(),
			std::exchange(DescriptorSetLayout, {}),
			nullptr);
	}
}
