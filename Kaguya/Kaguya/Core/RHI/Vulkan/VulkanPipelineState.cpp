#include "VulkanPipelineState.h"

static constexpr VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,
													VK_DYNAMIC_STATE_SCISSOR,
													VK_DYNAMIC_STATE_STENCIL_REFERENCE,
													VK_DYNAMIC_STATE_BLEND_CONSTANTS };

class VulkanPipelineParserCallbacks final : public IPipelineParserCallbacks
{
public:
	VulkanPipelineParserCallbacks() noexcept
	{
		ColorBlendStateCreateInfo				  = VkStruct<VkPipelineColorBlendStateCreateInfo>();
		ColorBlendStateCreateInfo.flags			  = 0;
		ColorBlendStateCreateInfo.logicOpEnable	  = VK_FALSE;
		ColorBlendStateCreateInfo.logicOp		  = VK_LOGIC_OP_MAX_ENUM;
		ColorBlendStateCreateInfo.attachmentCount = 8;
		ColorBlendStateCreateInfo.pAttachments	  = BlendStates;

		RasterizationStateCreateInfo = VkStruct<VkPipelineRasterizationStateCreateInfo>();

		DepthStencilStateCreateInfo = VkStruct<VkPipelineDepthStencilStateCreateInfo>();

		VertexInputStateCreateInfo = VkStruct<VkPipelineVertexInputStateCreateInfo>();

		InputAssemblyStateCreateInfo = VkStruct<VkPipelineInputAssemblyStateCreateInfo>();

		MultisampleStateCreateInfo						 = VkStruct<VkPipelineMultisampleStateCreateInfo>();
		MultisampleStateCreateInfo.sampleShadingEnable	 = VK_FALSE;
		MultisampleStateCreateInfo.rasterizationSamples	 = VK_SAMPLE_COUNT_1_BIT;
		MultisampleStateCreateInfo.minSampleShading		 = 1.0f;
		MultisampleStateCreateInfo.pSampleMask			 = nullptr;
		MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		MultisampleStateCreateInfo.alphaToOneEnable		 = VK_FALSE;

		// Default initialize for different states here in case they are not present in the stream
		BlendStateCb(BlendState());
		RasterizerStateCb(RasterizerState());
		DepthStencilStateCb(DepthStencilState());
	}

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

	void BlendStateCb(const BlendState& BlendState) override
	{
		for (size_t i = 0; i < std::size(BlendState.RenderTargets); ++i)
		{
			const auto& RenderTarget		   = BlendState.RenderTargets[i];
			BlendStates[i].blendEnable		   = RenderTarget.BlendEnable ? VK_TRUE : VK_FALSE;
			BlendStates[i].srcColorBlendFactor = ToVkBlendFactor(RenderTarget.SrcBlendRGB);
			BlendStates[i].dstColorBlendFactor = ToVkBlendFactor(RenderTarget.DstBlendRGB);
			BlendStates[i].colorBlendOp		   = ToVkBlendOp(RenderTarget.BlendOpRGB);
			BlendStates[i].srcAlphaBlendFactor = ToVkBlendFactor(RenderTarget.SrcBlendAlpha);
			BlendStates[i].dstAlphaBlendFactor = ToVkBlendFactor(RenderTarget.DstBlendAlpha);
			BlendStates[i].alphaBlendOp		   = ToVkBlendOp(RenderTarget.BlendOpAlpha);
			BlendStates[i].colorWriteMask |= RenderTarget.WriteMask.R ? VK_COLOR_COMPONENT_R_BIT : 0;
			BlendStates[i].colorWriteMask |= RenderTarget.WriteMask.G ? VK_COLOR_COMPONENT_G_BIT : 0;
			BlendStates[i].colorWriteMask |= RenderTarget.WriteMask.B ? VK_COLOR_COMPONENT_B_BIT : 0;
			BlendStates[i].colorWriteMask |= RenderTarget.WriteMask.A ? VK_COLOR_COMPONENT_A_BIT : 0;
		}

		MultisampleStateCreateInfo.alphaToCoverageEnable = BlendState.AlphaToCoverageEnable ? VK_TRUE : VK_FALSE;
	}

	void RasterizerStateCb(const RasterizerState& RasterizerState) override
	{
		RasterizationStateCreateInfo.depthClampEnable		 = VK_FALSE; // ?
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // ?
		RasterizationStateCreateInfo.polygonMode			 = ToVkFillMode(RasterizerState.FillMode);
		RasterizationStateCreateInfo.cullMode				 = ToVkCullMode(RasterizerState.CullMode);
		RasterizationStateCreateInfo.lineWidth				 = 1.0f;							// ?
		RasterizationStateCreateInfo.frontFace				 = VK_FRONT_FACE_COUNTER_CLOCKWISE; // ?
		RasterizationStateCreateInfo.depthBiasEnable		 = VK_FALSE;						// ?
		RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;							// ?
		RasterizationStateCreateInfo.depthBiasClamp			 = 0.0f;							// ?
		RasterizationStateCreateInfo.depthBiasSlopeFactor	 = 0.0f;							// ?
		RasterizationStateCreateInfo.lineWidth				 = 1.0f;							// ?
	}

	void DepthStencilStateCb(const DepthStencilState& DepthStencilState) override
	{
		DepthStencilStateCreateInfo.flags				  = 0;
		DepthStencilStateCreateInfo.depthTestEnable		  = DepthStencilState.DepthEnable ? VK_TRUE : VK_FALSE;
		DepthStencilStateCreateInfo.depthWriteEnable	  = DepthStencilState.DepthWrite ? VK_TRUE : VK_FALSE;
		DepthStencilStateCreateInfo.depthCompareOp		  = ToVkCompareOp(DepthStencilState.DepthFunc);
		DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		DepthStencilStateCreateInfo.minDepthBounds		  = 0.0f; // Optional
		DepthStencilStateCreateInfo.maxDepthBounds		  = 1.0f; // Optional
		DepthStencilStateCreateInfo.stencilTestEnable	  = VK_FALSE;
	}

	void InputLayoutCb(const InputLayout& InputLayout) override
	{
		UINT CurrentOffset = 0;
		for (const auto& Element : InputLayout.Elements)
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
		InputAssemblyStateCreateInfo.flags					= 0;
		InputAssemblyStateCreateInfo.topology				= ToVkPrimitiveTopology(PrimitiveTopology);
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	}

	void RenderPassCb(IRHIRenderPass* RenderPass) override
	{
		VkRenderPass = RenderPass->As<VulkanRenderPass>()->GetApiHandle();

		ColorBlendStateCreateInfo.attachmentCount = RenderPass->As<VulkanRenderPass>()->GetDesc().NumRenderTargets;
	}

	VkPipelineLayout Layout = VK_NULL_HANDLE;

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

	VkPipelineColorBlendAttachmentState BlendStates[8] = {};
	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo;

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;

	VkVertexInputBindingDescription				   BindingDescription = {};
	std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo		   VertexInputStateCreateInfo;

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo;

	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo;

	VkRenderPass VkRenderPass = VK_NULL_HANDLE;
};

VulkanPipelineState::VulkanPipelineState(VulkanDevice* Parent, const PipelineStateStreamDesc& Desc)
	: IRHIPipelineState(Parent)
{
	auto DynamicState			   = VkStruct<VkPipelineDynamicStateCreateInfo>();
	DynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(DynamicStates));
	DynamicState.pDynamicStates	   = DynamicStates;

	// Dummy viewport structure because Vulkan validation layer complains about
	// having valid viewportCount/scissorCount for dynamic viewport
	auto ViewportState			= VkStruct<VkPipelineViewportStateCreateInfo>();
	ViewportState.viewportCount = 1;
	ViewportState.pViewports	= nullptr;
	ViewportState.scissorCount	= 1;
	ViewportState.pScissors		= nullptr;

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
	GraphicsPipelineCreateInfo.pMultisampleState   = &Parser.MultisampleStateCreateInfo;
	GraphicsPipelineCreateInfo.pDepthStencilState  = &Parser.DepthStencilStateCreateInfo;
	GraphicsPipelineCreateInfo.pColorBlendState	   = &Parser.ColorBlendStateCreateInfo;
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
		vkDestroyPipeline(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(Pipeline, {}), nullptr);
	}
}
