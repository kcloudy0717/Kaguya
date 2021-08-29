#pragma once
#include "VulkanCommon.h"

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
	VkPipelineViewportStateCreateInfo			 ViewportState;
	VkPipelineRasterizationStateCreateInfo		 RasterizationState;
	VkPipelineColorBlendAttachmentState			 ColorBlendAttachmentState;
	VkPipelineMultisampleStateCreateInfo		 MultisampleState;
	VkPipelineDepthStencilStateCreateInfo		 DepthStencilState;
	VkPipelineColorBlendStateCreateInfo			 ColorBlendState;
	VkPipelineLayout							 PipelineLayout;
	VkRenderPass								 RenderPass = VK_NULL_HANDLE;

	VkGraphicsPipelineCreateInfo Create()
	{
		// This is a dummy ViewportState, all VulkanPipelineState will have dynamic viewport/scissor rects
		// this is needed to by pass validation layer
		ViewportState				= VkStruct<VkPipelineViewportStateCreateInfo>();
		ViewportState.viewportCount = 1;
		ViewportState.pViewports	= nullptr;
		ViewportState.scissorCount	= 1;
		ViewportState.pScissors		= nullptr;

		// setup dummy color blending. We aren't using transparent objects yet
		// the blending is just "no blend", but we do write to the color attachment
		ColorBlendState					= VkStruct<VkPipelineColorBlendStateCreateInfo>();
		ColorBlendState.logicOpEnable	= VK_FALSE;
		ColorBlendState.logicOp			= VK_LOGIC_OP_COPY;
		ColorBlendState.attachmentCount = 1;
		ColorBlendState.pAttachments	= &ColorBlendAttachmentState;

		// build the actual pipeline
		// we now use all of the info structs we have been writing into into this one to create the pipeline
		auto GraphicsPipelineCreateInfo				   = VkStruct<VkGraphicsPipelineCreateInfo>();
		GraphicsPipelineCreateInfo.flags			   = 0;
		GraphicsPipelineCreateInfo.stageCount		   = static_cast<uint32_t>(ShaderStages.size());
		GraphicsPipelineCreateInfo.pStages			   = ShaderStages.data();
		GraphicsPipelineCreateInfo.pVertexInputState   = &VertexInputState;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyState;
		GraphicsPipelineCreateInfo.pTessellationState  = nullptr;
		GraphicsPipelineCreateInfo.pViewportState	   = &ViewportState;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationState;
		GraphicsPipelineCreateInfo.pMultisampleState   = &MultisampleState;
		GraphicsPipelineCreateInfo.pDepthStencilState  = &DepthStencilState;
		GraphicsPipelineCreateInfo.pColorBlendState	   = &ColorBlendState;
		GraphicsPipelineCreateInfo.pDynamicState	   = nullptr;
		GraphicsPipelineCreateInfo.layout			   = PipelineLayout;
		GraphicsPipelineCreateInfo.renderPass		   = RenderPass;
		GraphicsPipelineCreateInfo.subpass			   = 0;
		GraphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.basePipelineIndex   = 0;
		return GraphicsPipelineCreateInfo;
	}
};

class VulkanPipelineState : public VulkanDeviceChild
{
public:
	VulkanPipelineState() noexcept = default;
	VulkanPipelineState(VulkanDevice* Parent, VulkanPipelineStateBuilder& Builder);
	VulkanPipelineState(VulkanDevice* Parent, const PipelineStateStreamDesc& Desc);
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
