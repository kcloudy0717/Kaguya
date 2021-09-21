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

	void SetGraphicsRootSignature(VulkanRootSignature* RootSignature);
	void SetComputeRootSignature(VulkanRootSignature* RootSignature);

	void SetGraphicsPipelineState(VulkanPipelineState* PipelineState);

	void SetViewports(UINT NumViewports, RHIViewport* Viewports, RHIRect* ScissorRects);

	void SetPushConstants(UINT Size, const void* SrcData);
	void SetConstantBufferView(UINT Binding, IRHIBuffer* Buffer);
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

	VkCommandBuffer		CommandBuffer	  = VK_NULL_HANDLE;
	VkPipelineBindPoint PipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	VkPipelineLayout	PipelineLayout	  = VK_NULL_HANDLE;
};
