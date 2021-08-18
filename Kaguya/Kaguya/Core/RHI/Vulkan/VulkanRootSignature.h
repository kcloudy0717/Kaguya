#pragma once
#include "VulkanCommon.h"

class VulkanRootSignatureBuilder
{
public:
	template<typename T>
	void Add32BitConstants()
	{
		VkPushConstantRange& PushConstantRange = PushConstantRanges.emplace_back();
		PushConstantRange.stageFlags		   = VK_SHADER_STAGE_ALL;
		PushConstantRange.offset			   = 0;
		PushConstantRange.size				   = sizeof(T);
	}

	VkPipelineLayoutCreateInfo Build() noexcept;

private:
	std::vector<VkPushConstantRange> PushConstantRanges;
};

class VulkanRootSignature : public VulkanDeviceChild
{
public:
	VulkanRootSignature() noexcept = default;
	VulkanRootSignature(VulkanDevice* Parent, VulkanRootSignatureBuilder& Builder);
	~VulkanRootSignature();

	VulkanRootSignature(VulkanRootSignature&& VulkanRootSignature)
		: VulkanDeviceChild(std::exchange(VulkanRootSignature.Parent, nullptr))
		, PipelineLayout(std::exchange(VulkanRootSignature.PipelineLayout, VK_NULL_HANDLE))
	{
	}
	VulkanRootSignature& operator=(VulkanRootSignature&& VulkanRootSignature)
	{
		if (this != &VulkanRootSignature)
		{
			Parent		   = std::exchange(VulkanRootSignature.Parent, nullptr);
			PipelineLayout = std::exchange(VulkanRootSignature.PipelineLayout, VK_NULL_HANDLE);
		}
		return *this;
	}

	NONCOPYABLE(VulkanRootSignature);

	operator VkPipelineLayout() const noexcept { return PipelineLayout; }

private:
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
};
