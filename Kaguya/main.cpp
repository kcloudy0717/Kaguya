// main.cpp : Defines the entry point for the application.
//
#include <Core/Application.h>
#include <Core/RHI/Vulkan/Adapter.h>
#include <Core/RHI/Vulkan/SwapChain.h>

class VulkanEngine : public Application
{
public:
	VulkanEngine() {}

	~VulkanEngine() {}

	bool Initialize() override
	{
		DeviceOptions Options	 = {};
		Options.EnableDebugLayer = true;
		Adapter.Initialize(Options);
		Adapter.InitializeDevice();

		CommandBuffer = Adapter.GetDevice().GetGraphicsQueue()->GetCommandBuffer();

		SwapChain.Initialize(GetWindowHandle(), &Adapter);

		InitDefaultRenderPass();
		InitFrameBuffers();
		InitSyncStructures();

		return true;
	}

	void Update(float DeltaTime) override
	{
		// wait until the GPU has finished rendering the last frame. Timeout of 1 second
		VERIFY_VULKAN_API(vkWaitForFences(Adapter.GetVkDevice(), 1, &RenderFence, true, 1000000000));
		VERIFY_VULKAN_API(vkResetFences(Adapter.GetVkDevice(), 1, &RenderFence));

		// request image from the swapchain, one second timeout
		uint32_t swapchainImageIndex = SwapChain.AcquireNextImage(PresentSemaphore);

		// now that we are sure that the commands finished executing, we can safely reset the command buffer to begin
		// recording again.
		VERIFY_VULKAN_API(vkResetCommandBuffer(CommandBuffer, 0));

		// naming it cmd for shorter writing
		VkCommandBuffer cmd = CommandBuffer;

		// begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan
		// know that
		auto CommandBufferBeginInfo	 = VkStruct<VkCommandBufferBeginInfo>();
		CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VERIFY_VULKAN_API(vkBeginCommandBuffer(cmd, &CommandBufferBeginInfo));

		// make a clear-color from frame number. This will flash with a 120*pi frame period.
		VkClearValue	clearValue;
		static uint32_t _frameNumber = 0;
		float			flash		 = abs(sin(_frameNumber / 120.f));
		clearValue.color			 = { { 0.0f, 0.0f, flash, 1.0f } };

		// start the main renderpass.
		// We will use the clear color from above, and the framebuffer of the index the swapchain gave us
		auto rpInfo				   = VkStruct<VkRenderPassBeginInfo>();
		rpInfo.renderPass		   = RenderPass;
		rpInfo.renderArea.offset.x = 0;
		rpInfo.renderArea.offset.y = 0;
		rpInfo.renderArea.extent   = SwapChain.GetExtent();
		rpInfo.framebuffer		   = Framebuffers[swapchainImageIndex];

		// connect clear values
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues	   = &clearValue;

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		// finalize the render pass
		vkCmdEndRenderPass(cmd);
		// finalize the command buffer (we can no longer add commands, but it can now be executed)
		VERIFY_VULKAN_API(vkEndCommandBuffer(cmd));

		// prepare the submission to the queue.
		// we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// we will signal the _renderSemaphore, to signal that rendering has finished

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		auto SubmitInfo				 = VkStruct<VkSubmitInfo>();
		SubmitInfo.pWaitDstStageMask = &waitStage;

		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores	  = &PresentSemaphore;

		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores	= &RenderSemaphore;

		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers	  = &cmd;

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VERIFY_VULKAN_API(
			vkQueueSubmit(Adapter.GetDevice().GetGraphicsQueue()->GetAPIHandle(), 1, &SubmitInfo, RenderFence));

		SwapChain.Present(RenderSemaphore);

		// increase the number of frames drawn
		_frameNumber++;
	}

	void Shutdown() override
	{
		VERIFY_VULKAN_API(vkWaitForFences(Adapter.GetVkDevice(), 1, &RenderFence, true, UINT64_MAX));

		vkDestroyFence(Adapter.GetVkDevice(), RenderFence, nullptr);
		vkDestroySemaphore(Adapter.GetVkDevice(), RenderSemaphore, nullptr);
		vkDestroySemaphore(Adapter.GetVkDevice(), PresentSemaphore, nullptr);

		vkDestroyRenderPass(Adapter.GetVkDevice(), RenderPass, nullptr);
		for (auto& Framebuffer : Framebuffers)
		{
			vkDestroyFramebuffer(Adapter.GetVkDevice(), Framebuffer, nullptr);
		}
	}

	void Resize(UINT Width, UINT Height) override {}

private:
	void InitDefaultRenderPass()
	{
		// the renderpass will use this color attachment.
		VkAttachmentDescription AttachmentDescription = {};
		// the attachment will have the format needed by the swapchain
		AttachmentDescription.format = SwapChain.GetFormat();
		// 1 sample, we won't be doing MSAA
		AttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		// we Clear when this attachment is loaded
		AttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// we keep the attachment stored when the renderpass ends
		AttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// we don't care about stencil
		AttachmentDescription.stencilLoadOp	 = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// we don't know or care about the starting layout of the attachment
		AttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// after the renderpass ends, the image has to be on a layout ready for display
		AttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference AttachmentReference = {};
		// attachment number will index into the pAttachments array in the parent renderpass itself
		AttachmentReference.attachment = 0;
		AttachmentReference.layout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription SubpassDescription = {};
		SubpassDescription.pipelineBindPoint	= VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pColorAttachments	= &AttachmentReference;

		auto RenderPassCreateInfo = VkStruct<VkRenderPassCreateInfo>();
		// connect the color attachment to the info
		RenderPassCreateInfo.attachmentCount = 1;
		RenderPassCreateInfo.pAttachments	 = &AttachmentDescription;
		// connect the subpass to the info
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pSubpasses	  = &SubpassDescription;

		VERIFY_VULKAN_API(vkCreateRenderPass(Adapter.GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass));
	}

	void InitFrameBuffers()
	{
		// create the framebuffers for the swapchain images. This will connect the render-pass to the images for
		// rendering
		auto FramebufferCreateInfo			  = VkStruct<VkFramebufferCreateInfo>();
		FramebufferCreateInfo.renderPass	  = RenderPass;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.width			  = SwapChain.GetWidth();
		FramebufferCreateInfo.height		  = SwapChain.GetHeight();
		FramebufferCreateInfo.layers		  = 1;

		// grab how many images we have in the swapchain
		const uint32_t ImageCount = SwapChain.GetImageCount();
		Framebuffers.resize(ImageCount);
		// create framebuffers for each of the swapchain image views
		for (uint32_t i = 0; i < ImageCount; i++)
		{
			const VkImageView Attachment[]	   = { SwapChain.GetImageView(i) };
			FramebufferCreateInfo.pAttachments = Attachment;
			VERIFY_VULKAN_API(
				vkCreateFramebuffer(Adapter.GetVkDevice(), &FramebufferCreateInfo, nullptr, &Framebuffers[i]));
		}
	}

	void InitSyncStructures()
	{
		// create synchronization structures
		auto FenceCreateInfo = VkStruct<VkFenceCreateInfo>();
		// we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU
		// command (for the first frame)
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VERIFY_VULKAN_API(vkCreateFence(Adapter.GetVkDevice(), &FenceCreateInfo, nullptr, &RenderFence));

		// for the semaphores we don't need any flags
		auto SemaphoreCreateInfo  = VkStruct<VkSemaphoreCreateInfo>();
		SemaphoreCreateInfo.flags = 0;

		VERIFY_VULKAN_API(vkCreateSemaphore(Adapter.GetVkDevice(), &SemaphoreCreateInfo, nullptr, &PresentSemaphore));
		VERIFY_VULKAN_API(vkCreateSemaphore(Adapter.GetVkDevice(), &SemaphoreCreateInfo, nullptr, &RenderSemaphore));
	}

private:
	Adapter	  Adapter;
	SwapChain SwapChain;

	VkRenderPass			   RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> Framebuffers;

	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

	VkSemaphore PresentSemaphore, RenderSemaphore;
	VkFence		RenderFence;
};

int main(int argc, char* argv[])
{
	try
	{
		Application::InitializeComponents();

		const ApplicationOptions AppOptions = { .Name = L"Vulkan", .Width = 1280, .Height = 720, .Maximize = true };

		VulkanEngine App;
		Application::Run(App, AppOptions);
	}
	catch (std::exception& Exception)
	{
		MessageBoxA(nullptr, Exception.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}
