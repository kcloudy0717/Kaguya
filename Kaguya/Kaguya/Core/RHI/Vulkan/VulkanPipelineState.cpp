#include "VulkanPipelineState.h"

VulkanPipelineState::VulkanPipelineState(VulkanDevice* Parent, VulkanPipelineStateBuilder& Builder)
	: VulkanDeviceChild(Parent)
{
	Pipeline = Builder.Create(Parent->GetVkDevice());
}

VulkanPipelineState::~VulkanPipelineState()
{
	if (Parent && Pipeline)
	{
		vkDestroyPipeline(Parent->GetVkDevice(), Pipeline, nullptr);
	}
}
