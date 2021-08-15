// main.cpp : Defines the entry point for the application.
//
#include <Core/Application.h>

#include <iostream>

class VulkanEngine : public Application
{
public:
	VulkanEngine() {}

	~VulkanEngine() {}

	bool Initialize() override
	{
		ShaderCompiler.Initialize();

		DeviceOptions Options	 = {};
		Options.EnableDebugLayer = true;
		Device.Initialize(Options);
		Device.InitializeDevice();

		CommandBuffer = Device.GetGraphicsQueue().GetCommandBuffer();

		SwapChain.Initialize(GetWindowHandle(), &Device);

		InitDefaultRenderPass();
		InitFrameBuffers();
		InitSyncStructures();
		InitPipelines();

		return true;
	}

	void Update(float DeltaTime) override
	{
		// wait until the GPU has finished rendering the last frame. Timeout of 1 second
		VERIFY_VULKAN_API(vkWaitForFences(Device.GetVkDevice(), 1, &RenderFence, true, 1000000000));
		VERIFY_VULKAN_API(vkResetFences(Device.GetVkDevice(), 1, &RenderFence));

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
		VkClearValue clearValue;
		clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

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

		// once we start adding rendering commands, they will go here

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, TrianglePipeline);
		vkCmdDraw(cmd, 3, 1, 0, 0);

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
		VERIFY_VULKAN_API(vkQueueSubmit(Device.GetGraphicsQueue().GetAPIHandle(), 1, &SubmitInfo, RenderFence));

		SwapChain.Present(RenderSemaphore);
	}

	void Shutdown() override
	{
		VERIFY_VULKAN_API(vkWaitForFences(Device.GetVkDevice(), 1, &RenderFence, true, UINT64_MAX));

		vkDestroyPipelineLayout(Device.GetVkDevice(), TrianglePipelineLayout, nullptr);

		vkDestroyShaderModule(Device.GetVkDevice(), PS, nullptr);
		vkDestroyShaderModule(Device.GetVkDevice(), VS, nullptr);

		vkDestroyFence(Device.GetVkDevice(), RenderFence, nullptr);
		vkDestroySemaphore(Device.GetVkDevice(), RenderSemaphore, nullptr);
		vkDestroySemaphore(Device.GetVkDevice(), PresentSemaphore, nullptr);

		vkDestroyRenderPass(Device.GetVkDevice(), RenderPass, nullptr);
		for (auto& Framebuffer : Framebuffers)
		{
			vkDestroyFramebuffer(Device.GetVkDevice(), Framebuffer, nullptr);
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

		VERIFY_VULKAN_API(vkCreateRenderPass(Device.GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass));
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
				vkCreateFramebuffer(Device.GetVkDevice(), &FramebufferCreateInfo, nullptr, &Framebuffers[i]));
		}
	}

	void InitSyncStructures()
	{
		// create synchronization structures
		auto FenceCreateInfo = VkStruct<VkFenceCreateInfo>();
		// we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU
		// command (for the first frame)
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VERIFY_VULKAN_API(vkCreateFence(Device.GetVkDevice(), &FenceCreateInfo, nullptr, &RenderFence));

		// for the semaphores we don't need any flags
		auto SemaphoreCreateInfo  = VkStruct<VkSemaphoreCreateInfo>();
		SemaphoreCreateInfo.flags = 0;

		VERIFY_VULKAN_API(vkCreateSemaphore(Device.GetVkDevice(), &SemaphoreCreateInfo, nullptr, &PresentSemaphore));
		VERIFY_VULKAN_API(vkCreateSemaphore(Device.GetVkDevice(), &SemaphoreCreateInfo, nullptr, &RenderSemaphore));
	}

	void InitPipelines()
	{
		if (!LoadShaderModule(EShaderType::Vertex, "Shaders/Vulkan/Triangle.vs.hlsl", &VS))
		{
			std::cout << "Error when building the triangle vertex shader module" << std::endl;
		}
		if (!LoadShaderModule(EShaderType::Pixel, "Shaders/Vulkan/Triangle.ps.hlsl", &PS))
		{
			std::cout << "Error when building the triangle fragment shader module" << std::endl;
		}

		// build the pipeline layout that controls the inputs/outputs of the shader
		// we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
		VkPipelineLayoutCreateInfo pipeline_layout_info = InitPipelineLayoutCreateInfo();

		VERIFY_VULKAN_API(
			vkCreatePipelineLayout(Device.GetVkDevice(), &pipeline_layout_info, nullptr, &TrianglePipelineLayout));

		// build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader
		// modules per stage
		VulkanPipelineStateBuilder pipelineBuilder;

		pipelineBuilder.ShaderStages.push_back(InitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, VS));
		pipelineBuilder.ShaderStages.push_back(InitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, PS));

		// vertex input controls how to read vertices from vertex buffers. We aren't using it yet
		pipelineBuilder.VertexInputState = InitPipelineVertexInputStateCreateInfo();

		// input assembly is the configuration for drawing triangle lists, strips, or individual points.
		// we are just going to draw triangle list
		pipelineBuilder.InputAssemblyState =
			InitPipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// build viewport and scissor from the swapchain extents
		pipelineBuilder.Viewport.x		  = 0.0f;
		pipelineBuilder.Viewport.y		  = 0.0f;
		pipelineBuilder.Viewport.width	  = (float)SwapChain.GetWidth();
		pipelineBuilder.Viewport.height	  = (float)SwapChain.GetHeight();
		pipelineBuilder.Viewport.minDepth = 0.0f;
		pipelineBuilder.Viewport.maxDepth = 1.0f;

		pipelineBuilder.ScissorRect.offset = { 0, 0 };
		pipelineBuilder.ScissorRect.extent = SwapChain.GetExtent();

		// configure the rasterizer to draw filled triangles
		pipelineBuilder.RasterizationState = InitPipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

		// we don't use multisampling, so just run the default one
		pipelineBuilder.MultisampleState = InitPipelineMultisampleStateCreateInfo();

		// a single blend attachment with no blending and writing to RGBA
		pipelineBuilder.ColorBlendAttachmentState = InitPipelineColorBlendAttachmentState();

		// use the triangle layout we created
		pipelineBuilder.PipelineLayout = TrianglePipelineLayout;
		pipelineBuilder.RenderPass	   = RenderPass;

		// finally build the pipeline
		TrianglePipeline = VulkanPipelineState(&Device, pipelineBuilder);
	}

	bool LoadShaderModule(EShaderType ShaderType, const std::filesystem::path& Path, VkShaderModule* pShaderModule)
	{
		Shader Shader = ShaderCompiler.SpirVCodeGen(ShaderType, ExecutableDirectory / Path, L"main", {});

		// create a new shader module, using the buffer we loaded
		auto ShaderModuleCreateInfo		= VkStruct<VkShaderModuleCreateInfo>();
		ShaderModuleCreateInfo.codeSize = Shader.GetBufferSize();
		ShaderModuleCreateInfo.pCode	= (uint32_t*)Shader.GetBufferPointer();

		// check that the creation goes well.
		VkShaderModule ShaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(Device.GetVkDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
		{
			return false;
		}
		*pShaderModule = ShaderModule;
		return true;
	}

private:
	ShaderCompiler ShaderCompiler;

	VulkanDevice	Device;
	VulkanSwapChain SwapChain;

	VkRenderPass			   RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> Framebuffers;

	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

	VkSemaphore PresentSemaphore = VK_NULL_HANDLE, RenderSemaphore = VK_NULL_HANDLE;
	VkFence		RenderFence = VK_NULL_HANDLE;

	VkShaderModule VS, PS;

	VkPipelineLayout TrianglePipelineLayout = VK_NULL_HANDLE;

	VulkanPipelineState TrianglePipeline;
};

int main(int argc, char* argv[])
{
	try
	{
		Application::InitializeComponents("Kaguya");

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

#if 0
// main.cpp : Defines the entry point for the application.
//
	#include <Core/Application.h>
	#include <Physics/PhysicsManager.h>
	#include <RenderCore/RenderCore.h>

	#include <Graphics/AssetManager.h>
	#include <Graphics/UI/HierarchyWindow.h>
	#include <Graphics/UI/ViewportWindow.h>
	#include <Graphics/UI/InspectorWindow.h>
	#include <Graphics/UI/AssetWindow.h>
	#include <Graphics/UI/ConsoleWindow.h>

	#include <Graphics/Renderer.h>
	#include <Graphics/PathIntegrator.h>

	#define RENDER_AT_1920x1080 0

class Editor : public Application
{
public:
	Editor() {}

	~Editor() {}

	bool Initialize() override
	{
		PhysicsManager::Initialize();
		RenderCore::Initialize();
		AssetManager::Initialize();

		std::string iniFile = (Application::ExecutableDirectory / "imgui.ini").string();

		ImGui::LoadIniSettingsFromDisk(iniFile.data());

		HierarchyWindow.SetContext(&World);
		InspectorWindow.SetContext(&World, {});
		AssetWindow.SetContext(&World);

		pRenderer = std::make_unique<PathIntegrator>(&World);
		pRenderer->OnInitialize();

		return true;
	}

	void Update(float DeltaTime) override
	{
		World.WorldState = EWorldState::EWorldState_Render;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow();

		ImGuizmo::AllowAxisFlip(false);

		ViewportWindow.SetContext(pRenderer->GetViewportDescriptor());
		// Update selected entity here in case Clear is called on HierarchyWindow to ensure entity is invalidated
		InspectorWindow.SetContext(&World, HierarchyWindow.GetSelectedEntity());

		HierarchyWindow.Render();
		ViewportWindow.Render();
		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.Render();

	#if RENDER_AT_1920x1080
		const uint32_t viewportWidth = 1920, viewportHeight = 1080;
	#else
		const uint32_t viewportWidth = ViewportWindow.Resolution.x, viewportHeight = ViewportWindow.Resolution.y;
	#endif

		World.Update(DeltaTime);

		// Render
		pRenderer->OnSetViewportResolution(viewportWidth, viewportHeight);

		pRenderer->OnRender();
	}

	void Shutdown() override
	{
		pRenderer->OnDestroy();
		pRenderer.reset();
		World.Clear();
		AssetManager::Shutdown();
		RenderCore::Shutdown();
		PhysicsManager::Shutdown();
	}

	void Resize(UINT Width, UINT Height) override { pRenderer->OnResize(Width, Height); }

private:
	HierarchyWindow HierarchyWindow;
	ViewportWindow	ViewportWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ConsoleWindow	ConsoleWindow;

	World					  World;
	std::unique_ptr<Renderer> pRenderer;
};

int main(int argc, char* argv[])
{
	try
	{
		Application::InitializeComponents("Kaguya");

		const ApplicationOptions AppOptions = { .Name	  = L"Kaguya",
												.Width	  = 1280,
												.Height	  = 720,
												.Maximize = true,
												.Icon	  = Application::ExecutableDirectory / "Assets/Kaguya.ico" };

		Editor App;
		Application::Run(App, AppOptions);
	}
	catch (std::exception& Exception)
	{
		MessageBoxA(nullptr, Exception.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}
#endif
