#include "VulkanDescriptorTable.h"

VulkanDescriptorTable::VulkanDescriptorTable(VulkanDevice* Parent, const DescriptorTableDesc& Desc)
	: IRHIDescriptorTable(Parent)
{
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	std::vector<VkDescriptorBindingFlagsEXT>  BindingFlags;

	for (auto [Type, NumDescriptors, Binding] : Desc.Ranges)
	{
		VkDescriptorSetLayoutBinding& VkBinding = Bindings.emplace_back();
		VkBinding.binding						= Binding;
		VkBinding.descriptorType				= ToVkDescriptorType(Type);
		VkBinding.descriptorCount				= NumDescriptors;
		VkBinding.stageFlags					= VK_SHADER_STAGE_ALL;
		BindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
	}

	auto BindingFlagsInfo				 = VkStruct<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>();
	BindingFlagsInfo.bindingCount		 = static_cast<uint32_t>(BindingFlags.size());
	BindingFlagsInfo.pBindingFlags		 = BindingFlags.data();
	auto DescriptorSetLayoutDesc		 = VkStruct<VkDescriptorSetLayoutCreateInfo>();
	DescriptorSetLayoutDesc.pNext		 = &BindingFlagsInfo;
	DescriptorSetLayoutDesc.flags		 = 0;
	DescriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(Bindings.size());
	DescriptorSetLayoutDesc.pBindings	 = Bindings.data();
	VERIFY_VULKAN_API(vkCreateDescriptorSetLayout(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetLayoutDesc,
		nullptr,
		&DescriptorSetLayout));
}

VulkanDescriptorTable::~VulkanDescriptorTable()
{
	if (Parent && DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(
			Parent->As<VulkanDevice>()->GetVkDevice(),
			std::exchange(DescriptorSetLayout, {}),
			nullptr);
	}
}
