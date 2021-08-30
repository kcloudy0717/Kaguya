#pragma once
#include "VulkanCommon.h"

class VulkanPipelineState final : public IRHIPipelineState
{
public:
	VulkanPipelineState() noexcept = default;
	VulkanPipelineState(VulkanDevice* Parent, const PipelineStateStreamDesc& Desc);
	~VulkanPipelineState() override;

	[[nodiscard]] VkPipeline GetApiHandle() const noexcept { return Pipeline; }

private:
	VkPipeline Pipeline = VK_NULL_HANDLE;
};
