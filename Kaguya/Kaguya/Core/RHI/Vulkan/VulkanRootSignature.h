#pragma once
#include "VulkanCommon.h"

class VulkanRootSignature final : public IRHIRootSignature
{
public:
	VulkanRootSignature() noexcept = default;
	VulkanRootSignature(VulkanDevice* Parent, const RootSignatureDesc& Desc);
	~VulkanRootSignature() override;

	[[nodiscard]] VkPipelineLayout GetApiHandle() const noexcept { return Handle; }

private:
	VkPipelineLayout Handle = VK_NULL_HANDLE;
};
