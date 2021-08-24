#pragma once
#include "vk_mem_alloc.h"
#include <Core/RHI/RHICommon.h>

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

class VulkanDevice;

class VulkanDeviceChild
{
public:
	VulkanDeviceChild() noexcept
		: Parent(nullptr)
	{
	}

	VulkanDeviceChild(VulkanDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentDevice() const noexcept -> VulkanDevice* { return Parent; }

	void SetParentDevice(VulkanDevice* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	VulkanDevice* Parent;
};

// clang-format off
template<typename T>	inline [[nodiscard]] auto VkStruct() -> T										{ static_assert("Not implemented"); return T(); }
template<>				inline [[nodiscard]] auto VkStruct() -> VkApplicationInfo						{ return { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkInstanceCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDeviceQueueCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDeviceCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkSubmitInfo							{ return { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkMemoryAllocateInfo					{ return { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkMappedMemoryRange						{ return { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkBindSparseInfo						{ return { .sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkFenceCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkSemaphoreCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkEventCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkQueryPoolCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkBufferCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkBufferViewCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkImageCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkImageViewCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkShaderModuleCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineCacheCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineShaderStageCreateInfo			{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineVertexInputStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineInputAssemblyStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineTessellationStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineViewportStateCreateInfo		{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineRasterizationStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineMultisampleStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineDepthStencilStateCreateInfo	{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineColorBlendStateCreateInfo		{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineDynamicStateCreateInfo		{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkGraphicsPipelineCreateInfo			{ return { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkComputePipelineCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPipelineLayoutCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkSamplerCreateInfo						{ return { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDescriptorSetLayoutCreateInfo			{ return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDescriptorPoolCreateInfo				{ return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkDescriptorSetAllocateInfo				{ return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkWriteDescriptorSet					{ return { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCopyDescriptorSet						{ return { .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkFramebufferCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkRenderPassCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandPoolCreateInfo					{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandBufferAllocateInfo				{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandBufferInheritanceInfo			{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkCommandBufferBeginInfo				{ return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkRenderPassBeginInfo					{ return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkBufferMemoryBarrier					{ return { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkImageMemoryBarrier					{ return { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkMemoryBarrier							{ return { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkSwapchainCreateInfoKHR				{ return { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkPresentInfoKHR						{ return { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR }; }
template<>				inline [[nodiscard]] auto VkStruct() -> VkWin32SurfaceCreateInfoKHR				{ return { .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR }; }

template<>				inline [[nodiscard]] auto VkStruct() -> VkDebugUtilsMessengerCreateInfoEXT		{ return { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT }; }
// clang-format on

struct QueueFamilyIndices
{
	bool IsValid() { return GraphicsFamily.has_value(); }

	std::optional<uint32_t> GraphicsFamily;
};

class VulkanInputLayout
{
public:
	VulkanInputLayout(uint32_t Stride);

	void AddVertexLayoutElement(UINT Location, VkFormat Format, uint32_t AlignedByteOffset)
	{
		VkVertexInputAttributeDescription& Desc = AttributeDescriptions.emplace_back();
		Desc.location							= Location;
		Desc.binding							= 0;
		Desc.format								= Format;
		Desc.offset								= AlignedByteOffset;
	}

	operator VkPipelineVertexInputStateCreateInfo() const noexcept;

private:
	std::vector<VkVertexInputBindingDescription>   BindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
	VkPipelineVertexInputStateCreateFlags		   StateCreateFlags = 0;
};
