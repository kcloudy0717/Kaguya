#include "VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(VulkanDevice* Parent, const RenderPassDesc& Desc)
	: IRHIRenderPass(Parent)
{
	bool ValidDepthStencilAttachment = Desc.DepthStencil.IsValid();

	UINT					NumAttachments	   = 0;
	VkAttachmentDescription Attachments[8 + 1] = {};

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

		// TODO FIX THIS
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

	VERIFY_VULKAN_API(vkCreateRenderPass(Parent->GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
	if (Parent && RenderPass)
	{
		vkDestroyRenderPass(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(RenderPass, {}), nullptr);
	}
}
