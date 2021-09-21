#pragma once
#include <Core/RHI/RHICore.h>

class VulkanRenderTarget final : public IRHIRenderTarget
{
public:
	VulkanRenderTarget(VulkanDevice* Parent, const RenderTargetDesc& Desc);
	~VulkanRenderTarget() override;

	[[nodiscard]] RenderTargetDesc GetDesc() const noexcept { return Desc; }
	[[nodiscard]] VkFramebuffer	   GetApiHandle() const noexcept { return Framebuffer; }

private:
	RenderTargetDesc Desc = {};

	VkFramebuffer Framebuffer = VK_NULL_HANDLE;
};
