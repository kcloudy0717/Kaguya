// main.cpp : Defines the entry point for the application.
//
#include <Core/Application.h>
#include <World/World.h>
#include <Physics/PhysicsManager.h>

#include <iostream>

struct UploadContext
{
	VkFence		  _uploadFence;
	VkCommandPool _commandPool;
};

struct MeshPushConstants
{
	DirectX::XMFLOAT4X4 Transform;
	UINT				Id;
};

struct UniformSceneConstants
{
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Projection;
	DirectX::XMFLOAT4X4 ViewProjection;
};

struct VulkanMesh
{
	std::shared_ptr<Asset::Mesh> Mesh;
	RefPtr<IRHIBuffer>			 VertexResource;
	RefPtr<IRHIBuffer>			 IndexResource;
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

		RHIBufferDesc BufferDesc = {};
		BufferDesc.SizeInBytes	 = sizeof(UniformSceneConstants);
		BufferDesc.Flags		 = RHIBufferFlag_ConstantBuffer;
		SceneConstants			 = Device.CreateBuffer(BufferDesc);

		RHITextureDesc TextureDesc =
			RHITextureDesc::Texture2D(ERHIFormat::D32, WindowWidth, WindowHeight, 1, RHITextureFlag_AllowDepthStencil);

		DepthBuffer = Device.CreateTexture(TextureDesc);

		InitDefaultRenderPass();
		InitFrameBuffers();
		InitSyncStructures();
		InitDescriptors();
		InitPipelines();

		LoadMeshes();
		InitScene();

		return true;
	}

	void Shutdown() override
	{
		VERIFY_VULKAN_API(vkWaitForFences(Device.GetVkDevice(), 1, &RenderFence, true, UINT64_MAX));
		vkDestroyFence(Device.GetVkDevice(), _uploadContext._uploadFence, nullptr);

		vkDestroyShaderModule(Device.GetVkDevice(), PS, nullptr);
		vkDestroyShaderModule(Device.GetVkDevice(), VS, nullptr);

		vkDestroyFence(Device.GetVkDevice(), RenderFence, nullptr);
		vkDestroySemaphore(Device.GetVkDevice(), RenderSemaphore, nullptr);
		vkDestroySemaphore(Device.GetVkDevice(), PresentSemaphore, nullptr);

		for (auto& Framebuffer : Framebuffers)
		{
			vkDestroyFramebuffer(Device.GetVkDevice(), Framebuffer, nullptr);
		}

		PhysicsManager::Shutdown();
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
				break;
			default:
				break;
			}
		}

		World.Update(DeltaTime);
		Camera& MainCamera = *World.ActiveCamera;

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

		// fill a GPU camera data struct
		UniformSceneConstants camData = {};
		XMStoreFloat4x4(&camData.View, XMMatrixTranspose(MainCamera.ViewMatrix));
		XMStoreFloat4x4(&camData.Projection, XMMatrixTranspose(MainCamera.ProjectionMatrix));
		XMStoreFloat4x4(&camData.ViewProjection, XMMatrixTranspose(MainCamera.ViewProjectionMatrix));

		SceneConstants->As<VulkanBuffer>()->Upload(
			[&camData](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, &camData, sizeof(UniformSceneConstants));
			});

		// begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan
		// know that
		auto CommandBufferBeginInfo	 = VkStruct<VkCommandBufferBeginInfo>();
		CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VERIFY_VULKAN_API(vkBeginCommandBuffer(cmd, &CommandBufferBeginInfo));

		VkClearValue ClearValues[2] = {};
		ClearValues[0].color		= { { 0.0f, 0.0f, 0.0f, 1.0f } };
		ClearValues[1].depthStencil = { 1.0f, 0 };

		// start the main renderpass.
		// We will use the clear color from above, and the framebuffer of the index the swapchain gave us
		auto RenderPassBeginInfo				= VkStruct<VkRenderPassBeginInfo>();
		RenderPassBeginInfo.renderPass			= RenderPass->As<VulkanRenderPass>()->GetApiHandle();
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderArea.extent	= SwapChain.GetExtent();
		RenderPassBeginInfo.framebuffer			= Framebuffers[swapchainImageIndex];
		RenderPassBeginInfo.clearValueCount		= 2;
		RenderPassBeginInfo.pClearValues		= ClearValues;

		vkCmdBeginRenderPass(cmd, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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
		VERIFY_VULKAN_API(vkQueueSubmit(Device.GetGraphicsQueue().GetApiHandle(), 1, &SubmitInfo, RenderFence));

		SwapChain.Present(RenderSemaphore);
	}

	void Resize(UINT Width, UINT Height) override {}

private:
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBuffer cmd = Device.GetCopyQueue().GetCommandBuffer();

		// begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan
		// know that
		auto CommandBufferBeginInfo	 = VkStruct<VkCommandBufferBeginInfo>();
		CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VERIFY_VULKAN_API(vkBeginCommandBuffer(cmd, &CommandBufferBeginInfo));
		function(cmd);
		VERIFY_VULKAN_API(vkEndCommandBuffer(cmd));

		auto SubmitInfo				  = VkStruct<VkSubmitInfo>();
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers	  = &cmd;

		VERIFY_VULKAN_API(
			vkQueueSubmit(Device.GetCopyQueue().GetApiHandle(), 1, &SubmitInfo, _uploadContext._uploadFence));

		VERIFY_VULKAN_API(vkWaitForFences(Device.GetVkDevice(), 1, &_uploadContext._uploadFence, true, UINT64_MAX));
		VERIFY_VULKAN_API(vkResetFences(Device.GetVkDevice(), 1, &_uploadContext._uploadFence));

		// clear the command pool. This will free the command buffer too
		VERIFY_VULKAN_API(vkResetCommandPool(Device.GetVkDevice(), _uploadContext._commandPool, 0));
	}

	void InitDefaultRenderPass()
	{
		RenderPassDesc Desc		= {};
		FLOAT		   Color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		Desc.AddRenderTarget({ .Format	   = SwapChain.GetFormat(),
							   .LoadOp	   = ELoadOp::Clear,
							   .StoreOp	   = EStoreOp::Store,
							   .ClearValue = ClearValue(SwapChain.GetFormat(), Color) });
		Desc.SetDepthStencil(
			{ .Format	  = DepthBuffer->As<VulkanTexture>()->GetDesc().format,
			  .LoadOp	  = ELoadOp::Clear,
			  .StoreOp	  = EStoreOp::Store,
			  .ClearValue = ClearValue(DepthBuffer->As<VulkanTexture>()->GetDesc().format, 1.0f, 0xFF) });

		RenderPass = Device.CreateRenderPass(Desc);
	}

	void InitFrameBuffers()
	{
		// create the framebuffers for the swapchain images. This will connect the render-pass to the images for
		// rendering
		auto FramebufferCreateInfo		 = VkStruct<VkFramebufferCreateInfo>();
		FramebufferCreateInfo.renderPass = RenderPass->As<VulkanRenderPass>()->GetApiHandle();
		FramebufferCreateInfo.width		 = SwapChain.GetWidth();
		FramebufferCreateInfo.height	 = SwapChain.GetHeight();
		FramebufferCreateInfo.layers	 = 1;

		// grab how many images we have in the swapchain
		const uint32_t ImageCount = SwapChain.GetImageCount();
		Framebuffers.resize(ImageCount);
		// create framebuffers for each of the swapchain image views
		for (uint32_t i = 0; i < ImageCount; i++)
		{
			const VkImageView Attachment[]		  = { SwapChain.GetImageView(i),
												  DepthBuffer->As<VulkanTexture>()->GetImageView() };
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

		FenceCreateInfo = VkStruct<VkFenceCreateInfo>();

		VERIFY_VULKAN_API(vkCreateFence(Device.GetVkDevice(), &FenceCreateInfo, nullptr, &_uploadContext._uploadFence));

		_uploadContext._commandPool = Device.GetCopyQueue().GetCommandPool();
	}

	void InitDescriptors()
	{
		{
			DescriptorTableDesc Desc = {};
			Desc.AddDescriptorRange({ .Type = EDescriptorType::ConstantBuffer, .NumDescriptors = 4096, .Binding = 0 });
			Desc.AddDescriptorRange({ .Type = EDescriptorType::Texture, .NumDescriptors = 4096, .Binding = 1 });
			Desc.AddDescriptorRange({ .Type = EDescriptorType::RWTexture, .NumDescriptors = 4096, .Binding = 2 });
			Desc.AddDescriptorRange({ .Type = EDescriptorType::Sampler, .NumDescriptors = 4096, .Binding = 3 });

			DescriptorTable = Device.CreateDescriptorTable(Desc);
		}

		{
			RootSignatureDesc Desc = {};
			Desc.AddPushConstant<MeshPushConstants>();
			Desc.DescriptorTables[0] = DescriptorTable.Get();
			Desc.NumDescriptorTables = 1;
			RootSignature			 = Device.CreateRootSignature(Desc);
		}

		DescriptorPoolDesc Desc = { .NumConstantBufferDescriptors  = 4096,
									.NumShaderResourceDescriptors  = 4096,
									.NumUnorderedAccessDescriptors = 4096,
									.NumSamplers				   = 4096,
									.DescriptorTable			   = DescriptorTable.Get() };

		DescriptorPool = Device.CreateDescriptorPool(Desc);

		DescriptorHandle Handle = DescriptorPool->AllocateDescriptorHandle(EDescriptorType::ConstantBuffer);
		Handle.Resource			= SceneConstants.Get();

		DescriptorPool->UpdateDescriptor(Handle);

		Handle = DescriptorPool->AllocateDescriptorHandle(EDescriptorType::Sampler);

		DescriptorPool->UpdateDescriptor(Handle);
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
		pipelineBuilder.PipelineLayout = RootSignature->As<VulkanRootSignature>()->GetApiHandle();
		pipelineBuilder.RenderPass	   = RenderPass->As<VulkanRenderPass>()->GetApiHandle();

		// finally build the pipeline
		MeshPipeline = VulkanPipelineState(&Device, pipelineBuilder);

		create_material(RootSignature->As<VulkanRootSignature>(), std::move(MeshPipeline), "Default");
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
		Asset::MeshMetadata MeshMetadata = {};
		MeshMetadata.Path				 = ExecutableDirectory / "Assets/models/Sphere.obj";
		SphereMesh.Mesh					 = MeshLoader.AsyncLoad(MeshMetadata);

		UploadMesh(SphereMesh);

		LoadImageFromPath("Assets/models/bedroom/textures/Teapot.tga");

		TextureDescriptor = DescriptorPool->AllocateDescriptorHandle(EDescriptorType::Texture);

		TextureDescriptor.Resource = Texture.Get();
		DescriptorPool->UpdateDescriptor(TextureDescriptor);
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
				RenderObject tri = {};
				tri.mesh		 = &SphereMesh;
				tri.material	 = get_material("Default");
				XMStoreFloat4x4(
					&tri.transformMatrix,
					DirectX::XMMatrixTranslation(float(x) * 2.5f, 0, float(y) * 2.5f));

				_renderables.push_back(tri);
			}
		}
	}

	void UploadMesh(VulkanMesh& Mesh)
	{
		RefPtr<IRHIBuffer> StagingBuffer;

		RHIBufferDesc StagingBufferDesc = {};
		StagingBufferDesc.HeapType		= ERHIHeapType::Upload;

		StagingBufferDesc.SizeInBytes = Mesh.Mesh->Vertices.size() * sizeof(Vertex);
		StagingBuffer				  = Device.CreateBuffer(StagingBufferDesc);
		StagingBuffer->As<VulkanBuffer>()->Upload(
			[&](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, Mesh.Mesh->Vertices.data(), StagingBufferDesc.SizeInBytes);
			});

		RHIBufferDesc VertexBufferDesc = {};
		VertexBufferDesc.SizeInBytes   = StagingBufferDesc.SizeInBytes;
		VertexBufferDesc.HeapType	   = ERHIHeapType::DeviceLocal;
		VertexBufferDesc.Flags		   = RHIBufferFlag_VertexBuffer;
		Mesh.VertexResource			   = Device.CreateBuffer(VertexBufferDesc);

		immediate_submit(
			[&](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size	   = VertexBufferDesc.SizeInBytes;
				vkCmdCopyBuffer(
					cmd,
					StagingBuffer->As<VulkanBuffer>()->GetApiHandle(),
					Mesh.VertexResource->As<VulkanBuffer>()->GetApiHandle(),
					1,
					&copy);
			});

		StagingBufferDesc.SizeInBytes = Mesh.Mesh->Indices.size() * sizeof(unsigned int);
		StagingBuffer				  = Device.CreateBuffer(StagingBufferDesc);
		StagingBuffer->As<VulkanBuffer>()->Upload(
			[&](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, Mesh.Mesh->Indices.data(), StagingBufferDesc.SizeInBytes);
			});

		RHIBufferDesc IndexBufferDesc = {};
		IndexBufferDesc.SizeInBytes	  = StagingBufferDesc.SizeInBytes;
		IndexBufferDesc.HeapType	  = ERHIHeapType::DeviceLocal;
		IndexBufferDesc.Flags		  = RHIBufferFlag_IndexBuffer;
		Mesh.IndexResource			  = Device.CreateBuffer(IndexBufferDesc);

		immediate_submit(
			[&](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size	   = IndexBufferDesc.SizeInBytes;
				vkCmdCopyBuffer(
					cmd,
					StagingBuffer->As<VulkanBuffer>()->GetApiHandle(),
					Mesh.IndexResource->As<VulkanBuffer>()->GetApiHandle(),
					1,
					&copy);
			});
	}

	void LoadImageFromPath(const std::filesystem::path& Path)
	{
		Asset::ImageMetadata Metadata = { .Path = ExecutableDirectory / Path };

		auto Image = ImageLoader.AsyncLoad(Metadata);

		void*		 pixel_ptr = Image->Image.GetPixels();
		VkDeviceSize imageSize = Image->Image.GetPixelsSize();

		VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

		RHIBufferDesc StagingBufferDesc = {};
		StagingBufferDesc.HeapType		= ERHIHeapType::Upload;

		StagingBufferDesc.SizeInBytes	 = imageSize;
		RefPtr<IRHIBuffer> StagingBuffer = Device.CreateBuffer(StagingBufferDesc);
		StagingBuffer->As<VulkanBuffer>()->Upload(
			[&](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, pixel_ptr, StagingBufferDesc.SizeInBytes);
			});

		RHITextureDesc TextureDesc = RHITextureDesc::Texture2D(
			ERHIFormat::RGBA8_UNORM,
			Image->Image.GetMetadata().width,
			Image->Image.GetMetadata().height,
			Image->Image.GetMetadata().mipLevels);

		Texture = Device.CreateTexture(TextureDesc);

		immediate_submit(
			[&](VkCommandBuffer cmd)
			{
				VkImageSubresourceRange range;
				range.aspectMask	 = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel	 = 0;
				range.levelCount	 = 1;
				range.baseArrayLayer = 0;
				range.layerCount	 = 1;

				VkImageMemoryBarrier imageBarrier_toTransfer = {};
				imageBarrier_toTransfer.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

				imageBarrier_toTransfer.oldLayout		 = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrier_toTransfer.newLayout		 = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrier_toTransfer.image			 = Texture->As<VulkanTexture>()->GetApiHandle();
				imageBarrier_toTransfer.subresourceRange = range;

				imageBarrier_toTransfer.srcAccessMask = 0;
				imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				// barrier the image into the transfer-receive layout
				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&imageBarrier_toTransfer);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset		 = 0;
				copyRegion.bufferRowLength	 = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask	   = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel	   = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount	   = 1;
				copyRegion.imageExtent = { TextureDesc.Width, TextureDesc.Height, TextureDesc.DepthOrArraySize };

				// copy the buffer into the image
				vkCmdCopyBufferToImage(
					cmd,
					StagingBuffer->As<VulkanBuffer>()->GetApiHandle(),
					Texture->As<VulkanTexture>()->GetApiHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

				imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				// barrier the image into the shader readable layout
				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&imageBarrier_toReadable);
			});
	}

private:
	ShaderCompiler ShaderCompiler;

	VulkanDevice Device;

	VulkanSwapChain SwapChain;

	RefPtr<IRHIRenderPass>	   RenderPass;
	std::vector<VkFramebuffer> Framebuffers;

	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	UploadContext	_uploadContext;

	VkSemaphore PresentSemaphore = VK_NULL_HANDLE, RenderSemaphore = VK_NULL_HANDLE;
	VkFence		RenderFence = VK_NULL_HANDLE;

	VkShaderModule VS, PS;

	RefPtr<IRHIDescriptorTable> DescriptorTable;
	RefPtr<IRHIDescriptorPool>	DescriptorPool;
	RefPtr<IRHIRootSignature>	RootSignature;
	VulkanPipelineState			MeshPipeline;
	VulkanMesh					SphereMesh;

	RefPtr<IRHITexture> DepthBuffer;

	RefPtr<IRHIBuffer>	SceneConstants;
	RefPtr<IRHITexture> Texture;
	DescriptorHandle	TextureDescriptor;

	World			 World;
	AsyncMeshLoader	 MeshLoader;
	AsyncImageLoader ImageLoader;

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

				VkDescriptorSet DescriptorSets[] = { DescriptorPool->As<VulkanDescriptorPool>()->GetDescriptorSet() };
				vkCmdBindDescriptorSets(
					cmd,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					object.material->RootSignature->GetApiHandle(),
					0,
					1,
					DescriptorSets,
					0,
					nullptr);
				lastMaterial = object.material;
			}

			MeshPushConstants constants = {};
			XMStoreFloat4x4(&constants.Transform, XMMatrixTranspose(XMLoadFloat4x4(&object.transformMatrix)));
			constants.Id = TextureDescriptor.Index;

			// upload the mesh to the GPU via push constants
			vkCmdPushConstants(
				cmd,
				object.material->RootSignature->GetApiHandle(),
				VK_SHADER_STAGE_ALL,
				0,
				sizeof(MeshPushConstants),
				&constants);

			// only bind the mesh if it's a different one from last bind
			if (object.mesh != lastMesh)
			{
				// bind the mesh vertex buffer with offset 0
				VkDeviceSize Offsets[]		 = { 0 };
				VkBuffer	 VertexBuffers[] = { object.mesh->VertexResource->As<VulkanBuffer>()->GetApiHandle() };
				vkCmdBindVertexBuffers(cmd, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(
					cmd,
					object.mesh->IndexResource->As<VulkanBuffer>()->GetApiHandle(),
					0,
					VK_INDEX_TYPE_UINT32);
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
