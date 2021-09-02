#pragma once
#include "VulkanCommon.h"

class VulkanRootSignature;
class VulkanPipelineState;

class VulkanCommandContext : public VulkanDeviceChild
{
public:
	using VulkanDeviceChild::VulkanDeviceChild;

	void OpenCommandList();
	void CloseCommandList();

	void SetGraphicsRootSignature(VulkanRootSignature* pRootSignature);
	void SetComputeRootSignature(VulkanRootSignature* pRootSignature);

	void SetGraphicsPipelineState(VulkanPipelineState* pPipelineState);

	void SetViewports(UINT NumViewports, RHIViewport* pViewports, RHIRect* pScissorRects);

	void SetPushConstants(UINT Size, const void* pSrcData);
	void SetDescriptorSets();

	void BeginRenderPass(IRHIRenderPass* RenderPass, IRHIRenderTarget* RenderTarget);

	void EndRenderPass();

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
