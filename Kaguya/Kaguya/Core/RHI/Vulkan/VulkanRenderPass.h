#pragma once
#include <Core/RHI/RHICore.h>

class VulkanRenderPass final : public IRHIRenderPass
{
public:
	VulkanRenderPass() noexcept = default;
	VulkanRenderPass(VulkanDevice* Parent, const RenderPassDesc& Desc);
	~VulkanRenderPass() override;

	[[nodiscard]] RenderPassDesc GetDesc() const noexcept { return Desc; }
	[[nodiscard]] VkRenderPass	 GetApiHandle() const noexcept { return RenderPass; }

private:
	RenderPassDesc Desc;

	VkRenderPass		  RenderPass		  = VK_NULL_HANDLE;
	VkRenderPassBeginInfo RenderPassBeginInfo = {};
};
