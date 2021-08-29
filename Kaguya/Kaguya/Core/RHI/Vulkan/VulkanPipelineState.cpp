#include "VulkanPipelineState.h"

static constexpr VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,
													VK_DYNAMIC_STATE_SCISSOR,
													VK_DYNAMIC_STATE_STENCIL_REFERENCE,
													VK_DYNAMIC_STATE_BLEND_CONSTANTS };

struct VulkanPipelineParserCallbacks final : IPipelineParserCallbacks
{
	void RootSignatureCb(IRHIRootSignature* RootSignature) override
	{
		Layout = RootSignature->As<VulkanRootSignature>()->GetApiHandle();
	}

	void VSCb(Shader* Shader) override
	{
		auto& VS  = ShaderStages.emplace_back(VkStruct<VkPipelineShaderStageCreateInfo>());
		VS.stage  = VK_SHADER_STAGE_VERTEX_BIT;
		VS.module = Shader->ShaderModule;
		VS.pName  = "main";
	}

	void PSCb(Shader* Shader) override
	{
		auto& PS  = ShaderStages.emplace_back(VkStruct<VkPipelineShaderStageCreateInfo>());
		PS.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
		PS.module = Shader->ShaderModule;
		PS.pName  = "main";
	}

	void DSCb(Shader* Shader) override { throw std::logic_error("The method or operation is not implemented."); }

	void HSCb(Shader* Shader) override { throw std::logic_error("The method or operation is not implemented."); }

	void GSCb(Shader* Shader) override { throw std::logic_error("The method or operation is not implemented."); }

	void CSCb(Shader* Shader) override { throw std::logic_error("The method or operation is not implemented."); }

	void BlendStateCb(const BlendState&) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void RasterizerStateCb(const RasterizerState& RasterizerState) override
	{
		RasterizationStateCreateInfo						 = VkStruct<VkPipelineRasterizationStateCreateInfo>();
		RasterizationStateCreateInfo.depthClampEnable		 = VK_FALSE; // ?
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // ?
		RasterizationStateCreateInfo.polygonMode			 = ToVkFillMode(RasterizerState.m_FillMode);
		RasterizationStateCreateInfo.cullMode				 = ToVkCullMode(RasterizerState.m_CullMode);
		RasterizationStateCreateInfo.lineWidth				 = 1.0f;					// ?
		RasterizationStateCreateInfo.frontFace				 = VK_FRONT_FACE_CLOCKWISE; // ?
		RasterizationStateCreateInfo.depthBiasEnable		 = VK_FALSE;				// ?
		RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;					// ?
		RasterizationStateCreateInfo.depthBiasClamp			 = 0.0f;					// ?
		RasterizationStateCreateInfo.depthBiasSlopeFactor	 = 0.0f;					// ?
		RasterizationStateCreateInfo.lineWidth				 = 1.0f;					// ?
	}

	void DepthStencilStateCb(const DepthStencilState& DepthStencilState) override
	{
		DepthStencilStateCreateInfo						  = VkStruct<VkPipelineDepthStencilStateCreateInfo>();
		DepthStencilStateCreateInfo.flags				  = 0;
		DepthStencilStateCreateInfo.depthTestEnable		  = DepthStencilState.m_DepthEnable ? VK_TRUE : VK_FALSE;
		DepthStencilStateCreateInfo.depthWriteEnable	  = DepthStencilState.m_DepthWrite ? VK_TRUE : VK_FALSE;
		DepthStencilStateCreateInfo.depthCompareOp		  = ToVkCompareOp(DepthStencilState.m_DepthFunc);
		DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		DepthStencilStateCreateInfo.minDepthBounds		  = 0.0f; // Optional
		DepthStencilStateCreateInfo.maxDepthBounds		  = 1.0f; // Optional
		DepthStencilStateCreateInfo.stencilTestEnable	  = VK_FALSE;
	}

	void InputLayoutCb(const InputLayout& InputLayout) override
	{
		VertexInputStateCreateInfo = VkStruct<VkPipelineVertexInputStateCreateInfo>();

		UINT CurrentOffset = 0;
		for (const auto& Element : InputLayout.m_InputElements)
		{
			VkVertexInputAttributeDescription& Attribute = AttributeDescriptions.emplace_back();
			Attribute.location							 = Element.Location;
			Attribute.binding							 = 0;
			Attribute.format							 = ToVkFormat(Element.Format);
			Attribute.offset							 = CurrentOffset;

			CurrentOffset += Element.Stride;
		}

		// we will have just 1 vertex buffer binding, with a per-vertex rate
		BindingDescription.binding	 = 0;
		BindingDescription.stride	 = CurrentOffset;
		BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		VertexInputStateCreateInfo.pVertexBindingDescriptions	 = &BindingDescription;
		VertexInputStateCreateInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(AttributeDescriptions.size());
		VertexInputStateCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();
	}

	void PrimitiveTopologyTypeCb(PrimitiveTopology PrimitiveTopology) override
	{
		InputAssemblyStateCreateInfo						= VkStruct<VkPipelineInputAssemblyStateCreateInfo>();
		InputAssemblyStateCreateInfo.flags					= 0;
		InputAssemblyStateCreateInfo.topology				= ToVkPrimitiveTopology(PrimitiveTopology);
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	}

	void RenderPassCb(IRHIRenderPass* RenderPass) override
	{
		VkRenderPass = RenderPass->As<VulkanRenderPass>()->GetApiHandle();
	}

	VkPipelineLayout Layout = VK_NULL_HANDLE;

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;

	VkVertexInputBindingDescription				   BindingDescription;
	std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo		   VertexInputStateCreateInfo;

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo;

	VkRenderPass VkRenderPass = VK_NULL_HANDLE;
};

VulkanPipelineState::VulkanPipelineState(VulkanDevice* Parent, VulkanPipelineStateBuilder& Builder)
	: VulkanDeviceChild(Parent)
{
	auto DynamicState			   = VkStruct<VkPipelineDynamicStateCreateInfo>();
	DynamicState.dynamicStateCount = std::size(DynamicStates);
	DynamicState.pDynamicStates	   = DynamicStates;

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = Builder.Create();
	GraphicsPipelineCreateInfo.pDynamicState				= &DynamicState;

	VERIFY_VULKAN_API(vkCreateGraphicsPipelines(
		Parent->GetVkDevice(),
		VK_NULL_HANDLE,
		1,
		&GraphicsPipelineCreateInfo,
		nullptr,
		&Pipeline));
}

VulkanPipelineState::VulkanPipelineState(VulkanDevice* Parent, const PipelineStateStreamDesc& Desc)
{
	auto DynamicState			   = VkStruct<VkPipelineDynamicStateCreateInfo>();
	DynamicState.dynamicStateCount = std::size(DynamicStates);
	DynamicState.pDynamicStates	   = DynamicStates;

	auto ViewportState			= VkStruct<VkPipelineViewportStateCreateInfo>();
	ViewportState.viewportCount = 1;
	ViewportState.pViewports	= nullptr;
	ViewportState.scissorCount	= 1;
	ViewportState.pScissors		= nullptr;

	auto MultisampleState				   = VkStruct<VkPipelineMultisampleStateCreateInfo>();
	MultisampleState.sampleShadingEnable   = VK_FALSE;
	MultisampleState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	MultisampleState.minSampleShading	   = 1.0f;
	MultisampleState.pSampleMask		   = nullptr;
	MultisampleState.alphaToCoverageEnable = VK_FALSE;
	MultisampleState.alphaToOneEnable	   = VK_FALSE;

	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
	ColorBlendAttachmentState.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachmentState.blendEnable = VK_FALSE;

	auto ColorBlendState			= VkStruct<VkPipelineColorBlendStateCreateInfo>();
	ColorBlendState.logicOpEnable	= VK_FALSE;
	ColorBlendState.logicOp			= VK_LOGIC_OP_COPY;
	ColorBlendState.attachmentCount = 1;
	ColorBlendState.pAttachments	= &ColorBlendAttachmentState;

	VulkanPipelineParserCallbacks Parser;
	RHIParsePipelineStream(Desc, &Parser);

	auto GraphicsPipelineCreateInfo				   = VkStruct<VkGraphicsPipelineCreateInfo>();
	GraphicsPipelineCreateInfo.flags			   = 0;
	GraphicsPipelineCreateInfo.stageCount		   = static_cast<uint32_t>(Parser.ShaderStages.size());
	GraphicsPipelineCreateInfo.pStages			   = Parser.ShaderStages.data();
	GraphicsPipelineCreateInfo.pVertexInputState   = &Parser.VertexInputStateCreateInfo;
	GraphicsPipelineCreateInfo.pInputAssemblyState = &Parser.InputAssemblyStateCreateInfo;
	GraphicsPipelineCreateInfo.pTessellationState  = nullptr;
	GraphicsPipelineCreateInfo.pViewportState	   = &ViewportState;
	GraphicsPipelineCreateInfo.pRasterizationState = &Parser.RasterizationStateCreateInfo;
	GraphicsPipelineCreateInfo.pMultisampleState   = &MultisampleState;
	GraphicsPipelineCreateInfo.pDepthStencilState  = &Parser.DepthStencilStateCreateInfo;
	GraphicsPipelineCreateInfo.pColorBlendState	   = &ColorBlendState;
	GraphicsPipelineCreateInfo.pDynamicState	   = &DynamicState;
	GraphicsPipelineCreateInfo.layout			   = Parser.Layout;
	GraphicsPipelineCreateInfo.renderPass		   = Parser.VkRenderPass;
	GraphicsPipelineCreateInfo.subpass			   = 0;
	GraphicsPipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	GraphicsPipelineCreateInfo.basePipelineIndex   = 0;

	VERIFY_VULKAN_API(vkCreateGraphicsPipelines(
		Parent->GetVkDevice(),
		VK_NULL_HANDLE,
		1,
		&GraphicsPipelineCreateInfo,
		nullptr,
		&Pipeline));
}

VulkanPipelineState::~VulkanPipelineState()
{
	if (Parent && Pipeline)
	{
		vkDestroyPipeline(Parent->GetVkDevice(), Pipeline, nullptr);
	}
}
