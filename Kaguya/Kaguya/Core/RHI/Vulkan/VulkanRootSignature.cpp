#include "VulkanRootSignature.h"

VkPipelineLayoutCreateInfo VulkanRootSignatureBuilder::Build(VkDevice Device)
{
	DescriptorSetLayouts.resize(BindingLayouts.size());
	for (size_t i = 0; i < BindingLayouts.size(); ++i)
	{
		auto BindingFlagsInfo		   = VkStruct<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>();
		BindingFlagsInfo.bindingCount  = static_cast<uint32_t>(BindingLayouts[i].BindingFlags.size());
		BindingFlagsInfo.pBindingFlags = BindingLayouts[i].BindingFlags.data();

		auto DescriptorSetLayoutDesc		 = VkStruct<VkDescriptorSetLayoutCreateInfo>();
		DescriptorSetLayoutDesc.pNext		 = BindingLayouts[i].Bindless ? &BindingFlagsInfo : nullptr;
		DescriptorSetLayoutDesc.flags		 = 0;
		DescriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(BindingLayouts[i].Bindings.size());
		DescriptorSetLayoutDesc.pBindings	 = BindingLayouts[i].Bindings.data();

		VERIFY_VULKAN_API(
			vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutDesc, nullptr, &DescriptorSetLayouts[i]));
	}

	auto PipelineLayoutCreateInfo					= VkStruct<VkPipelineLayoutCreateInfo>();
	PipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32_t>(DescriptorSetLayouts.size());
	PipelineLayoutCreateInfo.pSetLayouts			= DescriptorSetLayouts.data();
	PipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(PushConstantRanges.size());
	PipelineLayoutCreateInfo.pPushConstantRanges	= PushConstantRanges.data();
	return PipelineLayoutCreateInfo;
}

VulkanRootSignature::VulkanRootSignature(VulkanDevice* Parent, VulkanRootSignatureBuilder& Builder)
	: VulkanDeviceChild(Parent)
{
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = Builder.Build(Parent->GetVkDevice());

	DescriptorSetLayouts = Builder.DescriptorSetLayouts;

	VERIFY_VULKAN_API(
		vkCreatePipelineLayout(Parent->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));
}

VulkanRootSignature::~VulkanRootSignature()
{
	if (Parent && PipelineLayout)
	{
		for (auto& DescriptorLayout : DescriptorSetLayouts)
		{
			vkDestroyDescriptorSetLayout(Parent->GetVkDevice(), std::exchange(DescriptorLayout, {}), nullptr);
		}
		vkDestroyPipelineLayout(Parent->GetVkDevice(), std::exchange(PipelineLayout, {}), nullptr);
	}
}
