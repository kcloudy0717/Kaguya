#pragma once
#include "VulkanCommon.h"

class VulkanRootSignature;

class VulkanCommandContext : public VulkanDeviceChild
{
public:
	using VulkanDeviceChild::VulkanDeviceChild;

	void OpenCommandList();
	void CloseCommandList();

	void SetGraphicsRootSignature(VulkanRootSignature* pRootSignature);
	void SetComputeRootSignature(VulkanRootSignature* pRootSignature);

	void SetPushConstants(UINT Size, const void* pSrcData);
	void SetDescriptorSets();

	void BeginRenderPass(const VkRenderPassBeginInfo& RenderPassBeginInfo)
	{
		vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void EndRenderPass() { vkCmdEndRenderPass(CommandBuffer); }

	void DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);

	void DrawIndexedInstanced(
		UINT IndexCount,
		UINT InstanceCount,
		UINT StartIndexLocation,
		UINT BaseVertexLocation,
		UINT StartInstanceLocation);

	void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

	VkCommandBuffer		 CommandBuffer	   = VK_NULL_HANDLE;
	VkPipelineBindPoint	 PipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	VulkanRootSignature* RootSignature	   = VK_NULL_HANDLE;
};
