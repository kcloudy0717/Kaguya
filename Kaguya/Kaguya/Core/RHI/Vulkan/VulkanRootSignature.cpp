#include "VulkanRootSignature.h"

VkPipelineLayoutCreateInfo VulkanRootSignatureBuilder::Build() noexcept
{
	auto PipelineLayoutCreateInfo					= VkStruct<VkPipelineLayoutCreateInfo>();
	PipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(PushConstantRanges.size());
	PipelineLayoutCreateInfo.pPushConstantRanges	= PushConstantRanges.data();
	return PipelineLayoutCreateInfo;
}

VulkanRootSignature::VulkanRootSignature(VulkanDevice* Parent, VulkanRootSignatureBuilder& Builder)
	: VulkanDeviceChild(Parent)
{
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = Builder.Build();

	VERIFY_VULKAN_API(
		vkCreatePipelineLayout(Parent->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
}

VulkanRootSignature::~VulkanRootSignature()
{
	if (Parent && PipelineLayout)
	{
		vkDestroyPipelineLayout(Parent->GetVkDevice(), PipelineLayout, nullptr);
	}
}
