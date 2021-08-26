#pragma once
#include "VulkanCommon.h"

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(bool Bindless) noexcept
		: Bindless(Bindless)
	{
	}

	template<UINT Binding>
	void AddBinding(VkDescriptorType Type, UINT NumDescriptors, VkShaderStageFlags ShaderStageFlags)
	{
		VkDescriptorSetLayoutBinding& Range = Bindings.emplace_back();
		Range.binding						= Binding;
		Range.descriptorType				= Type;
		Range.descriptorCount				= NumDescriptors;
		Range.stageFlags					= ShaderStageFlags;
		if (Bindless)
		{
			BindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
		}
	}

	bool									  Bindless = false;
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	std::vector<VkDescriptorBindingFlagsEXT>  BindingFlags;
};

class VulkanRootSignatureBuilder
{
public:
	void AddDescriptorSetLayout(VulkanDescriptorSetLayout DescriptorTable)
	{
		BindingLayouts.push_back(DescriptorTable);
	}

	template<typename T>
	void Add32BitConstants()
	{
		VkPushConstantRange& PushConstantRange = PushConstantRanges.emplace_back();
		PushConstantRange.stageFlags		   = VK_SHADER_STAGE_ALL;
		PushConstantRange.offset			   = 0;
		PushConstantRange.size				   = sizeof(T);
	}

	VkPipelineLayoutCreateInfo Build(VkDevice Device);

private:
	friend class VulkanRootSignature;
	std::vector<VulkanDescriptorSetLayout> BindingLayouts;
	std::vector<VkDescriptorSetLayout>	   DescriptorSetLayouts;
	std::vector<VkPushConstantRange>	   PushConstantRanges;
};

class VulkanRootSignature : public VulkanDeviceChild
{
public:
	VulkanRootSignature() noexcept = default;
	VulkanRootSignature(VulkanDevice* Parent, VulkanRootSignatureBuilder& Builder);
	~VulkanRootSignature();

	VulkanRootSignature(VulkanRootSignature&& VulkanRootSignature) noexcept
		: VulkanDeviceChild(std::exchange(VulkanRootSignature.Parent, {}))
		, PipelineLayout(std::exchange(VulkanRootSignature.PipelineLayout, {}))
		, DescriptorSetLayouts(std::exchange(VulkanRootSignature.DescriptorSetLayouts, {}))
	{
	}
	VulkanRootSignature& operator=(VulkanRootSignature&& VulkanRootSignature) noexcept
	{
		if (this != &VulkanRootSignature)
		{
			Parent				 = std::exchange(VulkanRootSignature.Parent, {});
			PipelineLayout		 = std::exchange(VulkanRootSignature.PipelineLayout, {});
			DescriptorSetLayouts = std::exchange(VulkanRootSignature.DescriptorSetLayouts, {});
		}
		return *this;
	}

	NONCOPYABLE(VulkanRootSignature);

	operator VkPipelineLayout() const noexcept { return PipelineLayout; }

	[[nodiscard]] auto GetDescriptorSetLayout(size_t Index) -> VkDescriptorSetLayout
	{
		return DescriptorSetLayouts[Index];
	}

private:
	VkPipelineLayout				   PipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
};
