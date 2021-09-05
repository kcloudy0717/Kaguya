#include "VulkanRootSignature.h"

VulkanRootSignature::VulkanRootSignature(VulkanDevice* Parent, const RootSignatureDesc& Desc)
	: IRHIRootSignature(Parent)
{
	if (!Desc.DescriptorSetLayoutBindings.empty())
	{
		auto DescriptorSetLayoutCreateInfo = VkStruct<VkDescriptorSetLayoutCreateInfo>();
		// Setting this flag tells the descriptor set layouts that no actual descriptor sets are allocated but instead
		// pushed at command buffer creation time
		DescriptorSetLayoutCreateInfo.flags		   = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		DescriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(Desc.DescriptorSetLayoutBindings.size());
		DescriptorSetLayoutCreateInfo.pBindings	   = Desc.DescriptorSetLayoutBindings.data();
		VERIFY_VULKAN_API(vkCreateDescriptorSetLayout(
			Parent->GetVkDevice(),
			&DescriptorSetLayoutCreateInfo,
			nullptr,
			&PushDescriptorSetLayout));
	}

	std::vector<VkDescriptorSetLayout> Layouts;
	std::vector<VkPushConstantRange>   PushConstantRanges;

	Layouts.push_back(Parent->As<VulkanDevice>()->GetResourceDescriptorHeap().DescriptorSetLayout);
	Layouts.push_back(Parent->As<VulkanDevice>()->GetSamplerDescriptorHeap().DescriptorSetLayout);
	if (PushDescriptorSetLayout)
	{
		Layouts.push_back(PushDescriptorSetLayout);
	}

	PushConstantRanges.reserve(Desc.PushConstants.size());
	for (const auto& i : Desc.PushConstants)
	{
		VkPushConstantRange& PushConstant = PushConstantRanges.emplace_back();
		PushConstant.stageFlags			  = VK_SHADER_STAGE_ALL;
		PushConstant.offset				  = 0;
		PushConstant.size				  = i.Size;
	}

	auto PipelineLayoutCreateInfo					= VkStruct<VkPipelineLayoutCreateInfo>();
	PipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32_t>(Layouts.size());
	PipelineLayoutCreateInfo.pSetLayouts			= Layouts.data();
	PipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(PushConstantRanges.size());
	PipelineLayoutCreateInfo.pPushConstantRanges	= PushConstantRanges.data();

	VERIFY_VULKAN_API(vkCreatePipelineLayout(Parent->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &Handle));
}

VulkanRootSignature::~VulkanRootSignature()
{
	vkDestroyDescriptorSetLayout(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		std::exchange(PushDescriptorSetLayout, {}),
		nullptr);
	vkDestroyPipelineLayout(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(Handle, {}), nullptr);
}
