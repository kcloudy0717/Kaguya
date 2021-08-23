#pragma once
#include "VulkanCommon.h"

inline VkPipelineShaderStageCreateInfo InitPipelineShaderStageCreateInfo(
	VkShaderStageFlagBits ShaderStage,
	VkShaderModule		  ShaderModule)
{
	auto PipelineShaderStageCreateInfo = VkStruct<VkPipelineShaderStageCreateInfo>();
	// shader stage
	PipelineShaderStageCreateInfo.stage = ShaderStage;
	// module containing the code for this shader stage
	PipelineShaderStageCreateInfo.module = ShaderModule;
	// the entry point of the shader
	PipelineShaderStageCreateInfo.pName = "main";
	return PipelineShaderStageCreateInfo;
}

inline VkPipelineVertexInputStateCreateInfo InitPipelineVertexInputStateCreateInfo()
{
	auto PipelineVertexInputStateCreateInfo = VkStruct<VkPipelineVertexInputStateCreateInfo>();
	// no vertex bindings or attributes
	PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount   = 0;
	PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	return PipelineVertexInputStateCreateInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo InitPipelineInputAssemblyStateCreateInfo(
	VkPrimitiveTopology PrimitiveTopology)
{
	auto PipelineInputAssemblyStateCreateInfo	  = VkStruct<VkPipelineInputAssemblyStateCreateInfo>();
	PipelineInputAssemblyStateCreateInfo.topology = PrimitiveTopology;
	// we are not going to use primitive restart on the entire tutorial so leave it on false
	PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	return PipelineInputAssemblyStateCreateInfo;
}

inline VkPipelineRasterizationStateCreateInfo InitPipelineRasterizationStateCreateInfo(VkPolygonMode PolygonMode)
{
	auto PipelineRasterizationStateCreateInfo			  = VkStruct<VkPipelineRasterizationStateCreateInfo>();
	PipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	// discards all primitives before the rasterization stage if enabled which we don't want
	PipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;

	PipelineRasterizationStateCreateInfo.polygonMode = PolygonMode;
	PipelineRasterizationStateCreateInfo.lineWidth	 = 1.0f;
	// no backface cull
	PipelineRasterizationStateCreateInfo.cullMode  = VK_CULL_MODE_NONE;
	PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// no depth bias
	PipelineRasterizationStateCreateInfo.depthBiasEnable		 = VK_FALSE;
	PipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	PipelineRasterizationStateCreateInfo.depthBiasClamp			 = 0.0f;
	PipelineRasterizationStateCreateInfo.depthBiasSlopeFactor	 = 0.0f;

	return PipelineRasterizationStateCreateInfo;
}

inline VkPipelineMultisampleStateCreateInfo InitPipelineMultisampleStateCreateInfo()
{
	auto PipelineMultisampleStateCreateInfo				   = VkStruct<VkPipelineMultisampleStateCreateInfo>();
	PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	// multisampling defaulted to no multisampling (1 sample per pixel)
	PipelineMultisampleStateCreateInfo.rasterizationSamples	 = VK_SAMPLE_COUNT_1_BIT;
	PipelineMultisampleStateCreateInfo.minSampleShading		 = 1.0f;
	PipelineMultisampleStateCreateInfo.pSampleMask			 = nullptr;
	PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.alphaToOneEnable		 = VK_FALSE;
	return PipelineMultisampleStateCreateInfo;
}

inline VkPipelineDepthStencilStateCreateInfo InitPipelineDepthStencilStateCreateInfo(
	bool		bDepthTest,
	bool		bDepthWrite,
	VkCompareOp compareOp)
{
	auto PipelineDepthStencilStateCreateInfo				  = VkStruct<VkPipelineDepthStencilStateCreateInfo>();
	PipelineDepthStencilStateCreateInfo.depthTestEnable		  = bDepthTest ? VK_TRUE : VK_FALSE;
	PipelineDepthStencilStateCreateInfo.depthWriteEnable	  = bDepthWrite ? VK_TRUE : VK_FALSE;
	PipelineDepthStencilStateCreateInfo.depthCompareOp		  = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
	PipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	PipelineDepthStencilStateCreateInfo.minDepthBounds		  = 0.0f; // Optional
	PipelineDepthStencilStateCreateInfo.maxDepthBounds		  = 1.0f; // Optional
	PipelineDepthStencilStateCreateInfo.stencilTestEnable	  = VK_FALSE;
	return PipelineDepthStencilStateCreateInfo;
}

inline VkPipelineColorBlendAttachmentState InitPipelineColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {};
	PipelineColorBlendAttachmentState.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
	return PipelineColorBlendAttachmentState;
}

inline VkPipelineLayoutCreateInfo InitPipelineLayoutCreateInfo()
{
	auto PipelineLayoutCreateInfo = VkStruct<VkPipelineLayoutCreateInfo>();
	// empty defaults
	PipelineLayoutCreateInfo.flags					= 0;
	PipelineLayoutCreateInfo.setLayoutCount			= 0;
	PipelineLayoutCreateInfo.pSetLayouts			= nullptr;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	PipelineLayoutCreateInfo.pPushConstantRanges	= nullptr;
	return PipelineLayoutCreateInfo;
}

class VulkanPipelineStateBuilder
{
public:
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	VkPipelineVertexInputStateCreateInfo		 VertexInputState;
	VkPipelineInputAssemblyStateCreateInfo		 InputAssemblyState;
	VkViewport									 Viewport;
	VkRect2D									 ScissorRect;
	VkPipelineRasterizationStateCreateInfo		 RasterizationState;
	VkPipelineColorBlendAttachmentState			 ColorBlendAttachmentState;
	VkPipelineMultisampleStateCreateInfo		 MultisampleState;
	VkPipelineDepthStencilStateCreateInfo		 DepthStencilState;
	VkPipelineLayout							 PipelineLayout;
	VkRenderPass								 RenderPass;

	VkPipeline Create(VkDevice Device)
	{
		// make viewport state from our stored viewport and scissor.
		// at the moment we won't support multiple viewports or scissors
		auto ViewportStateCreateInfo		  = VkStruct<VkPipelineViewportStateCreateInfo>();
		ViewportStateCreateInfo.viewportCount = 1;
		ViewportStateCreateInfo.pViewports	  = &Viewport;
		ViewportStateCreateInfo.scissorCount  = 1;
		ViewportStateCreateInfo.pScissors	  = &ScissorRect;

		// setup dummy color blending. We aren't using transparent objects yet
		// the blending is just "no blend", but we do write to the color attachment
		auto ColorBlendStateCreateInfo			  = VkStruct<VkPipelineColorBlendStateCreateInfo>();
		ColorBlendStateCreateInfo.logicOpEnable	  = VK_FALSE;
		ColorBlendStateCreateInfo.logicOp		  = VK_LOGIC_OP_COPY;
		ColorBlendStateCreateInfo.attachmentCount = 1;
		ColorBlendStateCreateInfo.pAttachments	  = &ColorBlendAttachmentState;

		// build the actual pipeline
		// we now use all of the info structs we have been writing into into this one to create the pipeline
		auto GraphicsPipelineCreateInfo				   = VkStruct<VkGraphicsPipelineCreateInfo>();
		GraphicsPipelineCreateInfo.flags			   = 0;
		GraphicsPipelineCreateInfo.stageCount		   = static_cast<uint32_t>(ShaderStages.size());
		GraphicsPipelineCreateInfo.pStages			   = ShaderStages.data();
		GraphicsPipelineCreateInfo.pVertexInputState   = &VertexInputState;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
		GraphicsPipelineCreateInfo.pTessellationState  = nullptr;
		GraphicsPipelineCreateInfo.pViewportState	   = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
		GraphicsPipelineCreateInfo.pMultisampleState   = &MultisampleState;
		GraphicsPipelineCreateInfo.pDepthStencilState  = &DepthStencilState;
		GraphicsPipelineCreateInfo.pColorBlendState	   = &ColorBlendStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState	   = nullptr;
		GraphicsPipelineCreateInfo.layout			   = PipelineLayout;
		GraphicsPipelineCreateInfo.renderPass		   = RenderPass;
		GraphicsPipelineCreateInfo.subpass			   = 0;
		GraphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.basePipelineIndex   = 0;

		// it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK
		// case
		VkPipeline Pipeline;
		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &Pipeline) !=
			VK_SUCCESS)
		{
			return VK_NULL_HANDLE; // failed to create graphics pipeline
		}
		else
		{
			return Pipeline;
		}
	}
};

class VulkanPipelineState : public VulkanDeviceChild
{
public:
	VulkanPipelineState() noexcept = default;
	VulkanPipelineState(VulkanDevice* Parent, VulkanPipelineStateBuilder& Builder);
	~VulkanPipelineState();

	VulkanPipelineState(VulkanPipelineState&& VulkanPipelineState) noexcept
		: VulkanDeviceChild(std::exchange(VulkanPipelineState.Parent, nullptr))
		, Pipeline(std::exchange(VulkanPipelineState.Pipeline, VK_NULL_HANDLE))
	{
	}
	VulkanPipelineState& operator=(VulkanPipelineState&& VulkanPipelineState) noexcept
	{
		if (this != &VulkanPipelineState)
		{
			Parent	 = std::exchange(VulkanPipelineState.Parent, nullptr);
			Pipeline = std::exchange(VulkanPipelineState.Pipeline, VK_NULL_HANDLE);
		}
		return *this;
	}

	NONCOPYABLE(VulkanPipelineState);

	operator VkPipeline() const noexcept { return Pipeline; }

private:
	VkPipeline Pipeline = VK_NULL_HANDLE;
};
