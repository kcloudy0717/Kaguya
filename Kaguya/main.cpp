// main.cpp : Defines the entry point for the application.
//
#include <Core/Application.h>
#include <World/World.h>
#include <Physics/PhysicsManager.h>

#include <iostream>

struct VulkanMesh
{
	std::shared_ptr<Asset::Mesh> Mesh;
	VulkanBuffer				 VertexResource;
	VulkanBuffer				 IndexResource;
};

struct MeshPushConstants
{
	DirectX::XMFLOAT4	data;
	DirectX::XMFLOAT4X4 render_matrix;
};

struct VulkanMaterial
{
	VulkanRootSignature* RootSignature;
	VulkanPipelineState	 PipelineState;
};

struct RenderObject
{
	VulkanMesh*		mesh;
	VulkanMaterial* material;

	DirectX::XMFLOAT4X4 transformMatrix;
};

class VulkanEngine : public Application
{
public:
	VulkanEngine() {}

	~VulkanEngine() {}

	bool Initialize() override
	{
		PhysicsManager::Initialize();

		ShaderCompiler.Initialize();

		DeviceOptions Options	 = {};
		Options.EnableDebugLayer = true;
		Device.Initialize(Options);
		Device.InitializeDevice();

		CommandBuffer = Device.GetGraphicsQueue().GetCommandBuffer();

		SwapChain.Initialize(GetWindowHandle(), &Device);

		// depth image size will match the window
		VkExtent3D depthImageExtent = { WindowWidth, WindowHeight, 1 };

		// hardcoding the depth format to 32 bit float
		auto _depthFormat = VK_FORMAT_D32_SFLOAT;

		// the depth image will be an image with the format we selected and Depth Attachment usage flag
		VkImageCreateInfo dimg_info =
			ImageCreateInfo(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

		// for the depth image, we want to allocate it from GPU local memory
		VmaAllocationCreateInfo dimg_allocinfo = {};
		dimg_allocinfo.usage				   = VMA_MEMORY_USAGE_GPU_ONLY;
		dimg_allocinfo.requiredFlags		   = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		DepthBuffer = VulkanTexture(&Device, dimg_info, dimg_allocinfo);

		// build an image-view for the depth image to use for rendering
		VkImageViewCreateInfo dview_info = ImageViewCreateInfo(_depthFormat, DepthBuffer, VK_IMAGE_ASPECT_DEPTH_BIT);
		VERIFY_VULKAN_API(vkCreateImageView(Device.GetVkDevice(), &dview_info, nullptr, &_depthImageView));

		InitDefaultRenderPass();
		InitFrameBuffers();
		InitSyncStructures();
		InitPipelines();

		LoadMeshes();
		InitScene();

		return true;
	}

	void Update(float DeltaTime) override
	{
		while (auto Key = InputHandler.Keyboard.ReadKey())
		{
			if (!Key->IsPressed())
			{
				continue;
			}

			switch (Key->Code)
			{
			case VK_ESCAPE:
				InputHandler.RawInputEnabled = !InputHandler.RawInputEnabled;
			}
		}

		world.Update(DeltaTime);
		Camera& MainCamera = *world.ActiveCamera;

		MeshPushConstants PushConstants = {};
		XMStoreFloat4x4(&PushConstants.render_matrix, XMMatrixTranspose(MainCamera.ViewProjectionMatrix));

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

		VkClearValue depthClear		  = {};
		depthClear.depthStencil.depth = 1.0f;

		// start the main renderpass.
		// We will use the clear color from above, and the framebuffer of the index the swapchain gave us
		auto rpInfo				   = VkStruct<VkRenderPassBeginInfo>();
		rpInfo.renderPass		   = RenderPass;
		rpInfo.renderArea.offset.x = 0;
		rpInfo.renderArea.offset.y = 0;
		rpInfo.renderArea.extent   = SwapChain.GetExtent();
		rpInfo.framebuffer		   = Framebuffers[swapchainImageIndex];

		// connect clear values
		VkClearValue ClearValues[] = { clearValue, depthClear };
		rpInfo.clearValueCount	   = 2;
		rpInfo.pClearValues		   = ClearValues;

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		draw_objects(cmd, _renderables.data(), _renderables.size());

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

		vkDestroyImageView(Device.GetVkDevice(), _depthImageView, nullptr);

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

		PhysicsManager::Shutdown();
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

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format					 = DepthBuffer.GetDesc().format;
		depth_attachment.samples				 = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp					 = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp				 = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.stencilLoadOp			 = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.stencilStoreOp			 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout			 = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout			 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment			   = 1;
		depth_attachment_ref.layout				   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference AttachmentReference = {};
		// attachment number will index into the pAttachments array in the parent renderpass itself
		AttachmentReference.attachment = 0;
		AttachmentReference.layout	   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription SubpassDescription	   = {};
		SubpassDescription.pipelineBindPoint	   = VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.colorAttachmentCount	   = 1;
		SubpassDescription.pColorAttachments	   = &AttachmentReference;
		SubpassDescription.pDepthStencilAttachment = &depth_attachment_ref;

		VkAttachmentDescription attachments[2] = { AttachmentDescription, depth_attachment };

		auto RenderPassCreateInfo = VkStruct<VkRenderPassCreateInfo>();
		// connect the color attachment to the info
		RenderPassCreateInfo.attachmentCount = 2;
		RenderPassCreateInfo.pAttachments	 = attachments;
		// connect the subpass to the info
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pSubpasses	  = &SubpassDescription;

		VERIFY_VULKAN_API(vkCreateRenderPass(Device.GetVkDevice(), &RenderPassCreateInfo, nullptr, &RenderPass));
	}

	void InitFrameBuffers()
	{
		// create the framebuffers for the swapchain images. This will connect the render-pass to the images for
		// rendering
		auto FramebufferCreateInfo		 = VkStruct<VkFramebufferCreateInfo>();
		FramebufferCreateInfo.renderPass = RenderPass;
		FramebufferCreateInfo.width		 = SwapChain.GetWidth();
		FramebufferCreateInfo.height	 = SwapChain.GetHeight();
		FramebufferCreateInfo.layers	 = 1;

		// grab how many images we have in the swapchain
		const uint32_t ImageCount = SwapChain.GetImageCount();
		Framebuffers.resize(ImageCount);
		// create framebuffers for each of the swapchain image views
		for (uint32_t i = 0; i < ImageCount; i++)
		{
			const VkImageView Attachment[]		  = { SwapChain.GetImageView(i), _depthImageView };
			FramebufferCreateInfo.attachmentCount = std::size(Attachment);
			FramebufferCreateInfo.pAttachments	  = Attachment;
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
		if (!LoadShaderModule(EShaderType::Vertex, ExecutableDirectory / "Shaders/Vulkan/Triangle.vs.hlsl", &VS))
		{
			std::cout << "Error when building the triangle vertex shader module" << std::endl;
		}
		if (!LoadShaderModule(EShaderType::Pixel, ExecutableDirectory / "Shaders/Vulkan/Triangle.ps.hlsl", &PS))
		{
			std::cout << "Error when building the triangle fragment shader module" << std::endl;
		}

		// build the pipeline layout that controls the inputs/outputs of the shader
		// we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
		VulkanRootSignatureBuilder Builder;
		Builder.Add32BitConstants<MeshPushConstants>();

		RootSignature = VulkanRootSignature(&Device, Builder);

		// build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader
		// modules per stage
		VulkanPipelineStateBuilder pipelineBuilder;

		pipelineBuilder.ShaderStages.push_back(InitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, VS));
		pipelineBuilder.ShaderStages.push_back(InitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, PS));

		// vertex input controls how to read vertices from vertex buffers. We aren't using it yet
		VulkanInputLayout InputLayout(sizeof(Vertex));
		InputLayout.AddVertexLayoutElement(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position));
		InputLayout.AddVertexLayoutElement(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TextureCoord));
		InputLayout.AddVertexLayoutElement(2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal));

		pipelineBuilder.VertexInputState = InputLayout;

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

		pipelineBuilder.DepthStencilState =
			InitPipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

		// a single blend attachment with no blending and writing to RGBA
		pipelineBuilder.ColorBlendAttachmentState = InitPipelineColorBlendAttachmentState();

		// use the triangle layout we created
		pipelineBuilder.PipelineLayout = RootSignature;
		pipelineBuilder.RenderPass	   = RenderPass;

		// finally build the pipeline
		MeshPipeline = VulkanPipelineState(&Device, pipelineBuilder);

		create_material(&RootSignature, std::move(MeshPipeline), "Default");
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

	void LoadMeshes()
	{
		TriangleMesh.Mesh = std::make_shared<Asset::Mesh>();
		TriangleMesh.Mesh->Vertices.resize(3);
		TriangleMesh.Mesh->Vertices[0].Position = { 1.f, 1.f, 0.5f };
		TriangleMesh.Mesh->Vertices[1].Position = { -1.f, 1.f, 0.5f };
		TriangleMesh.Mesh->Vertices[2].Position = { 0.f, -1.f, 0.5f };
		TriangleMesh.Mesh->Vertices[0].Normal	= { 0.f, 1.f, 0.0f }; // pure green
		TriangleMesh.Mesh->Vertices[1].Normal	= { 0.f, 1.f, 0.0f }; // pure green
		TriangleMesh.Mesh->Vertices[2].Normal	= { 0.f, 1.f, 0.0f }; // pure green
		TriangleMesh.Mesh->Indices				= { 0, 1, 2 };

		Asset::MeshMetadata MeshMetadata = {};
		MeshMetadata.Path				 = ExecutableDirectory / "Assets/models/Sphere.obj";
		SphereMesh.Mesh					 = MeshLoader.AsyncLoad(MeshMetadata);

		UploadMesh(TriangleMesh);
		UploadMesh(SphereMesh);
	}

	void InitScene()
	{
		RenderObject monkey;
		monkey.mesh		= &SphereMesh;
		monkey.material = get_material("Default");
		XMStoreFloat4x4(&monkey.transformMatrix, DirectX::XMMatrixIdentity());

		_renderables.push_back(monkey);

		for (int x = -20; x <= 20; x++)
		{
			for (int y = -20; y <= 20; y++)
			{
				RenderObject tri;
				tri.mesh	 = &TriangleMesh;
				tri.material = get_material("Default");
				XMStoreFloat4x4(&tri.transformMatrix, DirectX::XMMatrixTranslation(x, 0, y));

				_renderables.push_back(tri);
			}
		}
	}

	void UploadMesh(VulkanMesh& Mesh)
	{
		auto BufferCreateInfo = VkStruct<VkBufferCreateInfo>();

		// let the VMA library know that this data should be writeable by CPU, but also readable by GPU
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage					 = VMA_MEMORY_USAGE_CPU_TO_GPU;

		BufferCreateInfo.size  = Mesh.Mesh->Vertices.size() * sizeof(Vertex);
		BufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		Mesh.VertexResource	   = VulkanBuffer(&Device, BufferCreateInfo, vmaallocInfo);
		Mesh.VertexResource.Upload(
			[&](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, Mesh.Mesh->Vertices.data(), BufferCreateInfo.size);
			});

		BufferCreateInfo.size  = Mesh.Mesh->Indices.size() * sizeof(unsigned int);
		BufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		Mesh.IndexResource	   = VulkanBuffer(&Device, BufferCreateInfo, vmaallocInfo);
		Mesh.IndexResource.Upload(
			[&](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, Mesh.Mesh->Indices.data(), BufferCreateInfo.size);
			});
	}

private:
	ShaderCompiler ShaderCompiler;

	VulkanDevice Device;

	VulkanSwapChain SwapChain;

	VkRenderPass			   RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> Framebuffers;

	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

	VkSemaphore PresentSemaphore = VK_NULL_HANDLE, RenderSemaphore = VK_NULL_HANDLE;
	VkFence		RenderFence = VK_NULL_HANDLE;

	VkShaderModule VS, PS;

	VulkanRootSignature RootSignature;
	VulkanPipelineState MeshPipeline;
	VulkanMesh			TriangleMesh;
	VulkanMesh			SphereMesh;

	VulkanTexture DepthBuffer;
	VkImageView	  _depthImageView;

	World			world;
	AsyncMeshLoader MeshLoader;

	// default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, VulkanMaterial> _materials;
	std::unordered_map<std::string, VulkanMesh>		_meshes;

	// create material and add it to the map
	VulkanMaterial* create_material(
		VulkanRootSignature*  RootSignature,
		VulkanPipelineState&& PipelineState,
		const std::string&	  name)
	{
		VulkanMaterial mat;
		mat.RootSignature = RootSignature;
		mat.PipelineState = std::move(PipelineState);
		_materials[name]  = std::move(mat);
		return &_materials[name];
	}

	// returns nullptr if it can't be found
	VulkanMaterial* get_material(const std::string& name)
	{
		// search for the object, and return nullptr if not found
		auto it = _materials.find(name);
		if (it == _materials.end())
		{
			return nullptr;
		}
		else
		{
			return &it->second;
		}
	}

	// returns nullptr if it can't be found
	VulkanMesh* get_mesh(const std::string& name)
	{
		auto it = _meshes.find(name);
		if (it == _meshes.end())
		{
			return nullptr;
		}
		else
		{
			return &it->second;
		}
	}

	// our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count)
	{
		VulkanMesh*		lastMesh	 = nullptr;
		VulkanMaterial* lastMaterial = nullptr;
		for (int i = 0; i < count; i++)
		{
			RenderObject& object = first[i];

			// only bind the pipeline if it doesn't match with the already bound one
			if (object.material != lastMaterial)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->PipelineState);
				lastMaterial = object.material;
			}

			MeshPushConstants constants;
			XMStoreFloat4x4(
				&constants.render_matrix,
				XMMatrixTranspose(XMMatrixMultiply(
					XMLoadFloat4x4(&object.transformMatrix),
					world.ActiveCamera->ViewProjectionMatrix)));

			// upload the mesh to the GPU via push constants
			vkCmdPushConstants(
				cmd,
				*object.material->RootSignature,
				VK_SHADER_STAGE_ALL,
				0,
				sizeof(MeshPushConstants),
				&constants);

			// only bind the mesh if it's a different one from last bind
			if (object.mesh != lastMesh)
			{
				// bind the mesh vertex buffer with offset 0
				VkDeviceSize Offsets[]		 = { 0 };
				VkBuffer	 VertexBuffers[] = { object.mesh->VertexResource };
				vkCmdBindVertexBuffers(cmd, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(cmd, object.mesh->IndexResource, 0, VK_INDEX_TYPE_UINT32);
				lastMesh = object.mesh;
			}
			// we can now draw
			vkCmdDrawIndexed(cmd, object.mesh->Mesh->Indices.size(), 1, 0, 0, 0);
		}
	}
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
