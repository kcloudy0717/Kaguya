#pragma once
#include "VulkanCommon.h"

// class VulkanRootSignatureBuilder
//{
// public:
//	void AddDescriptorSetLayout(VulkanDescriptorSetLayout DescriptorTable)
//	{
//		BindingLayouts.push_back(DescriptorTable);
//	}
//
//	template<typename T>
//	void Add32BitConstants()
//	{
//		VkPushConstantRange& PushConstantRange = PushConstantRanges.emplace_back();
//		PushConstantRange.stageFlags		   = VK_SHADER_STAGE_ALL;
//		PushConstantRange.offset			   = 0;
//		PushConstantRange.size				   = sizeof(T);
//	}
//
//	VkPipelineLayoutCreateInfo Build(VkDevice Device);
//
// private:
//	friend class VulkanRootSignature;
//	std::vector<VulkanDescriptorSetLayout> BindingLayouts;
//	std::vector<VkDescriptorSetLayout>	   DescriptorSetLayouts;
//	std::vector<VkPushConstantRange>	   PushConstantRanges;
//};

class VulkanRootSignature final : public IRHIRootSignature
{
public:
	VulkanRootSignature() noexcept = default;
	VulkanRootSignature(VulkanDevice* Parent, const RootSignatureDesc& Desc);
	~VulkanRootSignature() override;

	[[nodiscard]] VkPipelineLayout GetApiHandle() const noexcept { return PipelineLayout; }

private:
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
};
