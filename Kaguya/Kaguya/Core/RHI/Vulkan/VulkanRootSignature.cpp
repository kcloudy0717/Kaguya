#include "VulkanRootSignature.h"

VulkanRootSignature::VulkanRootSignature(VulkanDevice* Parent, const RootSignatureDesc& Desc)
	: IRHIRootSignature(Parent)
{
	std::vector<VkDescriptorSetLayout> Layouts(Desc.NumDescriptorTables);
	std::vector<VkPushConstantRange>   PushConstantRanges;
	PushConstantRanges.reserve(Desc.PushConstants.size());
	for (UINT i = 0; i < Desc.NumDescriptorTables; ++i)
	{
		Layouts[i] = Desc.DescriptorTables[i]->As<VulkanDescriptorTable>()->GetApiHandle();
	}
	for (size_t i = 0; i < Desc.PushConstants.size(); ++i)
	{
		VkPushConstantRange& PushConstant = PushConstantRanges.emplace_back();
		PushConstant.stageFlags			  = VK_SHADER_STAGE_ALL;
		PushConstant.offset				  = 0;
		PushConstant.size				  = Desc.PushConstants[i].Size;
	}

	auto PipelineLayoutCreateInfo					= VkStruct<VkPipelineLayoutCreateInfo>();
	PipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32_t>(Layouts.size());
	PipelineLayoutCreateInfo.pSetLayouts			= Layouts.data();
	PipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(PushConstantRanges.size());
	PipelineLayoutCreateInfo.pPushConstantRanges	= PushConstantRanges.data();

	VERIFY_VULKAN_API(
		vkCreatePipelineLayout(Parent->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
}

VulkanRootSignature::~VulkanRootSignature()
{
	if (Parent && PipelineLayout)
	{
		vkDestroyPipelineLayout(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(PipelineLayout, {}), nullptr);
	}
}
