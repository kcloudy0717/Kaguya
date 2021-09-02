#include "VulkanRenderTarget.h"

VulkanRenderTarget::VulkanRenderTarget(VulkanDevice* Parent, const RenderTargetDesc& Desc)
	: IRHIRenderTarget(Parent)
	, Desc(Desc)
{
	uint32_t	NumAttachments	   = 0;
	VkImageView Attachments[8 + 1] = {};

	for (UINT i = 0; i < Desc.NumRenderTargets; ++i)
	{
		Attachments[NumAttachments++] = Desc.RenderTargets[i]->As<VulkanTexture>()->GetImageView();
	}
	if (Desc.DepthStencil)
	{
		Attachments[NumAttachments++] = Desc.DepthStencil->As<VulkanTexture>()->GetImageView();
	}

	auto FramebufferCreateInfo		 = VkStruct<VkFramebufferCreateInfo>();
	FramebufferCreateInfo.renderPass = Desc.RenderPass->As<VulkanRenderPass>()->GetApiHandle();
	FramebufferCreateInfo.width		 = Desc.Width;
	FramebufferCreateInfo.height	 = Desc.Height;
	FramebufferCreateInfo.layers	 = 1;

	FramebufferCreateInfo.attachmentCount = NumAttachments;
	FramebufferCreateInfo.pAttachments	  = Attachments;
	VERIFY_VULKAN_API(vkCreateFramebuffer(Parent->GetVkDevice(), &FramebufferCreateInfo, nullptr, &Framebuffer));
}

VulkanRenderTarget::~VulkanRenderTarget()
{
	if (Parent && Framebuffer)
	{
		vkDestroyFramebuffer(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(Framebuffer, {}), nullptr);
	}
}
