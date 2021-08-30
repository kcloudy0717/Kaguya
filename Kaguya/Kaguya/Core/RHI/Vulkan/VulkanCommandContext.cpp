#include "VulkanCommandContext.h"

void VulkanCommandContext::OpenCommandList()
{
	constexpr VkCommandBufferResetFlags Flags = 0;
	VERIFY_VULKAN_API(vkResetCommandBuffer(CommandBuffer, Flags));

	auto CommandBufferBeginInfo				= VkStruct<VkCommandBufferBeginInfo>();
	CommandBufferBeginInfo.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;

	VERIFY_VULKAN_API(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
}

void VulkanCommandContext::CloseCommandList()
{
	VERIFY_VULKAN_API(vkEndCommandBuffer(CommandBuffer));
}

void VulkanCommandContext::SetGraphicsRootSignature(VulkanRootSignature* pRootSignature)
{
	PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	RootSignature	  = pRootSignature;
}

void VulkanCommandContext::SetComputeRootSignature(VulkanRootSignature* pRootSignature)
{
	PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	RootSignature	  = pRootSignature;
}

void VulkanCommandContext::SetGraphicsPipelineState(VulkanPipelineState* pPipelineState)
{
	PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	const VkPipeline Pipeline = pPipelineState->GetApiHandle();
	vkCmdBindPipeline(CommandBuffer, PipelineBindPoint, Pipeline);
}

void VulkanCommandContext::SetViewports(UINT NumViewports, RHIViewport* pViewports, RHIRect* pScissorRects)
{
	VkViewport VkViewports[8]	 = {};
	VkRect2D   VkScissorRects[8] = {};

	for (UINT ViewportIndex = 0; ViewportIndex < NumViewports; ++ViewportIndex)
	{
		VkViewports[ViewportIndex].x		= pViewports[ViewportIndex].TopLeftX;
		VkViewports[ViewportIndex].y		= pViewports[ViewportIndex].TopLeftY;
		VkViewports[ViewportIndex].width	= pViewports[ViewportIndex].Width;
		VkViewports[ViewportIndex].height	= pViewports[ViewportIndex].Height;
		VkViewports[ViewportIndex].minDepth = pViewports[ViewportIndex].MinDepth;
		VkViewports[ViewportIndex].maxDepth = pViewports[ViewportIndex].MaxDepth;

		VkScissorRects[ViewportIndex].offset.x		= pScissorRects[ViewportIndex].Left;
		VkScissorRects[ViewportIndex].offset.y		= pScissorRects[ViewportIndex].Top;
		VkScissorRects[ViewportIndex].extent.width	= pScissorRects[ViewportIndex].Right;
		VkScissorRects[ViewportIndex].extent.height = pScissorRects[ViewportIndex].Bottom;
	}
	vkCmdSetViewport(CommandBuffer, 0, NumViewports, VkViewports);
	vkCmdSetScissor(CommandBuffer, 0, NumViewports, VkScissorRects);
}

void VulkanCommandContext::SetPushConstants(UINT Size, const void* pSrcData)
{
	const VkPipelineLayout PipelineLayout = RootSignature->GetApiHandle();
	vkCmdPushConstants(CommandBuffer, PipelineLayout, VK_SHADER_STAGE_ALL, 0, Size, pSrcData);
}

void VulkanCommandContext::SetDescriptorSets()
{
	const VkPipelineLayout PipelineLayout	= RootSignature->GetApiHandle();
	const VkDescriptorSet  DescriptorSets[] = { GetParentDevice()->GetResourceDescriptorHeap().DescriptorSet,
												GetParentDevice()->GetSamplerDescriptorHeap().DescriptorSet };
	vkCmdBindDescriptorSets(CommandBuffer, PipelineBindPoint, PipelineLayout, 0, 2, DescriptorSets, 0, nullptr);
}

void VulkanCommandContext::DrawInstanced(
	UINT VertexCount,
	UINT InstanceCount,
	UINT StartVertexLocation,
	UINT StartInstanceLocation)
{
	vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void VulkanCommandContext::DrawIndexedInstanced(
	UINT IndexCount,
	UINT InstanceCount,
	UINT StartIndexLocation,
	UINT BaseVertexLocation,
	UINT StartInstanceLocation)
{
	vkCmdDrawIndexed(
		CommandBuffer,
		IndexCount,
		InstanceCount,
		StartIndexLocation,
		static_cast<int32_t>(BaseVertexLocation),
		StartInstanceLocation);
}

void VulkanCommandContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	vkCmdDispatch(CommandBuffer, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}
