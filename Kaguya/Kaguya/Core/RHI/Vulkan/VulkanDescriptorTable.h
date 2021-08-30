#pragma once
#include <Core/RHI/RHICore.h>

class VulkanDescriptorTable final : public IRHIDescriptorTable
{
public:
	VulkanDescriptorTable() noexcept = default;
	VulkanDescriptorTable(VulkanDevice* Parent, const DescriptorTableDesc& Desc);
	~VulkanDescriptorTable() override;

	[[nodiscard]] VkDescriptorSetLayout GetApiHandle() const noexcept { return DescriptorSetLayout; }

private:
	VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
};
