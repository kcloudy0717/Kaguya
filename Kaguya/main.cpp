// main.cpp : Defines the entry point for the application.

#define VULKAN_PLAYGROUND 1

#if VULKAN_PLAYGROUND

	#include <Core/Application.h>
	#include <World/World.h>
	#include <Physics/PhysicsManager.h>

	#include <iostream>

struct MeshPushConstants
{
	DirectX::XMFLOAT4X4 Transform;
	UINT				Id1;
	UINT				Id2;
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
	VulkanPipelineState* PipelineState;
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
		InitDescriptors();
		InitPipelines();

		// 1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize PoolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
											 { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
											 { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
											 { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType						 = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags						 = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets					 = 1000;
		pool_info.poolSizeCount				 = std::size(PoolSizes);
		pool_info.pPoolSizes				 = PoolSizes;

		VERIFY_VULKAN_API(vkCreateDescriptorPool(Device.GetVkDevice(), &pool_info, nullptr, &ImguiDescriptorPool));

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance					= Device.GetVkInstance();
		init_info.PhysicalDevice			= Device.GetVkPhysicalDevice();
		init_info.Device					= Device.GetVkDevice();
		init_info.Queue						= Device.GetGraphicsQueue().GetApiHandle();
		init_info.DescriptorPool			= ImguiDescriptorPool;
		init_info.MinImageCount				= 3;
		init_info.ImageCount				= 3;
		init_info.MSAASamples				= VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, RenderPass->As<VulkanRenderPass>()->GetApiHandle());
		GraphicsImmediate(
			[&](VkCommandBuffer CommandBuffer)
			{
				ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
			});
		// clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		LoadMeshes();
		InitScene();

		return true;
	}

	void Shutdown() override
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device.GetVkDevice(), ImguiDescriptorPool, nullptr);

		for (auto& Framebuffer : Framebuffers)
		{
			vkDestroyFramebuffer(Device.GetVkDevice(), Framebuffer, nullptr);
		}

		PhysicsManager::Shutdown();
	}

	void Update(float DeltaTime) override
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow();

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

		uint32_t swapchainImageIndex = SwapChain.AcquireNextImage();

		VulkanCommandContext& Context = Device.GetGraphicsQueue().GetCommandContext(0);
		Context.OpenCommandList();

		// fill a GPU camera data struct
		UniformSceneConstants UniformSceneConstants = {};
		XMStoreFloat4x4(&UniformSceneConstants.View, XMMatrixTranspose(MainCamera.ViewMatrix));
		XMStoreFloat4x4(&UniformSceneConstants.Projection, XMMatrixTranspose(MainCamera.ProjectionMatrix));
		XMStoreFloat4x4(&UniformSceneConstants.ViewProjection, XMMatrixTranspose(MainCamera.ViewProjectionMatrix));

		SceneConstants->As<VulkanBuffer>()->Upload(
			[&UniformSceneConstants](void* CPUVirtualAddress)
			{
				memcpy(CPUVirtualAddress, &UniformSceneConstants, sizeof(UniformSceneConstants));
			});

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

		Context.BeginRenderPass(RenderPassBeginInfo);

		RHIViewport Viewport	= RHIViewport((float)SwapChain.GetWidth(), (float)SwapChain.GetHeight());
		RHIRect		ScissorRect = { 0, 0, SwapChain.GetWidth(), SwapChain.GetHeight() };
		Context.SetViewports(1, &Viewport, &ScissorRect);

		draw_objects(Context, _renderables.data(), _renderables.size());

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Context.CommandBuffer);

		Context.EndRenderPass();

		Context.CloseCommandList();

		VulkanCommandSyncPoint SyncPoint = Device.GetGraphicsQueue().ExecuteCommandLists(1, &Context, false);

		SwapChain.Present();

		SyncPoint.WaitForCompletion();
	}

	void Resize(UINT Width, UINT Height) override {}

private:
	void Upload(std::function<void(VkCommandBuffer CommandBuffer)>&& Function)
	{
		VulkanCommandContext& Context = Device.GetCopyQueue().GetCommandContext(0);
		Context.OpenCommandList();
		Function(Context.CommandBuffer);
		Context.CloseCommandList();
		Device.GetCopyQueue().ExecuteCommandLists(1, &Context, true);
	}

	void GraphicsImmediate(std::function<void(VkCommandBuffer CommandBuffer)>&& Function)
	{
		VulkanCommandContext& Context = Device.GetGraphicsQueue().GetCommandContext(0);
		Context.OpenCommandList();
		Function(Context.CommandBuffer);
		Context.CloseCommandList();
		Device.GetGraphicsQueue().ExecuteCommandLists(1, &Context, true);
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

	void InitDescriptors()
	{
		{
			RootSignatureDesc Desc = {};
			Desc.AddPushConstant<MeshPushConstants>();
			RootSignature = Device.CreateRootSignature(Desc);
		}

		DescriptorHandle Handle =
			Device.GetResourceDescriptorHeap().AllocateDescriptorHandle(EDescriptorType::ConstantBuffer);
		Handle.Resource = SceneConstants.Get();

		Device.GetResourceDescriptorHeap().UpdateDescriptor(Handle);

		Handle = Device.GetSamplerDescriptorHeap().AllocateDescriptorHandle(EDescriptorType::Sampler);

		Device.GetSamplerDescriptorHeap().UpdateDescriptor(Handle);
	}

	void InitPipelines()
	{
		Shader VS = ShaderCompiler.SpirVCodeGen(
			Device.GetVkDevice(),
			EShaderType::Vertex,
			ExecutableDirectory / "Shaders/Vulkan/Triangle.vs.hlsl",
			L"main",
			{});
		Shader PS = ShaderCompiler.SpirVCodeGen(
			Device.GetVkDevice(),
			EShaderType::Pixel,
			ExecutableDirectory / "Shaders/Vulkan/Triangle.ps.hlsl",
			L"main",
			{});

		struct PipelineStateStream
		{
			PipelineStateStreamRootSignature	 RootSignature;
			PipelineStateStreamVS				 VS;
			PipelineStateStreamPS				 PS;
			PipelineStateStreamInputLayout		 InputLayout;
			PipelineStateStreamPrimitiveTopology PrimitiveTopology;
			PipelineStateStreamRenderPass		 RenderPass;
		} Stream;

		InputLayout InputLayout;
		InputLayout.AddVertexLayoutElement("POSITION", 0, 0, ERHIFormat::RGB32_FLOAT, sizeof(Vertex::Position));
		InputLayout.AddVertexLayoutElement("TEXCOORD", 0, 1, ERHIFormat::RG32_FLOAT, sizeof(Vertex::TextureCoord));
		InputLayout.AddVertexLayoutElement("NORMAL", 0, 2, ERHIFormat::RGB32_FLOAT, sizeof(Vertex::Normal));

		Stream.RootSignature	 = RootSignature.Get();
		Stream.VS				 = &VS;
		Stream.PS				 = &PS;
		Stream.InputLayout		 = InputLayout;
		Stream.PrimitiveTopology = PrimitiveTopology::Triangle;
		Stream.RenderPass		 = RenderPass.Get();

		MeshPipeline = Device.CreatePipelineState(
			{ .SizeInBytes = sizeof(PipelineStateStream), .pPipelineStateSubobjectStream = &Stream });

		vkDestroyShaderModule(Device.GetVkDevice(), VS.ShaderModule, nullptr);
		vkDestroyShaderModule(Device.GetVkDevice(), PS.ShaderModule, nullptr);

		create_material(RootSignature->As<VulkanRootSignature>(), MeshPipeline->As<VulkanPipelineState>(), "Default");
	}

	void LoadMeshes()
	{
		Asset::MeshMetadata MeshMetadata = {};
		MeshMetadata.Path				 = ExecutableDirectory / "Assets/models/Sphere.obj";
		SphereMesh.Mesh					 = MeshLoader.AsyncLoad(MeshMetadata);

		UploadMesh(SphereMesh);

		LoadImageFromPath("Assets/models/bedroom/textures/Teapot.tga");

		TextureDescriptor1 = Device.GetResourceDescriptorHeap().AllocateDescriptorHandle(EDescriptorType::Texture);
		TextureDescriptor1.Resource = Texture.Get();
		Device.GetResourceDescriptorHeap().UpdateDescriptor(TextureDescriptor1);
	}

	void InitScene()
	{
		RenderObject monkey = {};
		monkey.mesh			= &SphereMesh;
		monkey.material		= get_material("Default");
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

		Upload(
			[&](VkCommandBuffer CommandBuffer)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size	   = VertexBufferDesc.SizeInBytes;
				vkCmdCopyBuffer(
					CommandBuffer,
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

		Upload(
			[&](VkCommandBuffer CommandBuffer)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size	   = IndexBufferDesc.SizeInBytes;
				vkCmdCopyBuffer(
					CommandBuffer,
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

		Upload(
			[&](VkCommandBuffer CommandBuffer)
			{
				VkImageSubresourceRange range;
				range.aspectMask	 = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel	 = 0;
				range.levelCount	 = 1;
				range.baseArrayLayer = 0;
				range.layerCount	 = 1;

				auto Barrier				= VkStruct<VkImageMemoryBarrier>();
				Barrier.srcAccessMask		= 0;
				Barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
				Barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
				Barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				Barrier.image				= Texture->As<VulkanTexture>()->GetApiHandle();
				Barrier.subresourceRange	= range;

				// barrier the image into the transfer-receive layout
				vkCmdPipelineBarrier(
					CommandBuffer,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&Barrier);

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
					CommandBuffer,
					StagingBuffer->As<VulkanBuffer>()->GetApiHandle(),
					Texture->As<VulkanTexture>()->GetApiHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				Barrier.oldLayout	  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				Barrier.newLayout	  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				// barrier the image into the shader readable layout
				vkCmdPipelineBarrier(
					CommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&Barrier);
			});
	}

private:
	ShaderCompiler ShaderCompiler;

	VulkanDevice	 Device;
	VkDescriptorPool ImguiDescriptorPool;

	VulkanSwapChain SwapChain;

	RefPtr<IRHIRenderPass>	   RenderPass;
	std::vector<VkFramebuffer> Framebuffers;

	RefPtr<IRHIRootSignature> RootSignature;
	RefPtr<IRHIPipelineState> MeshPipeline;
	VulkanMesh				  SphereMesh;

	RefPtr<IRHITexture> DepthBuffer;

	RefPtr<IRHIBuffer>	SceneConstants;
	RefPtr<IRHITexture> Texture;
	DescriptorHandle	TextureDescriptor1;

	World			 World;
	AsyncMeshLoader	 MeshLoader;
	AsyncImageLoader ImageLoader;

	// default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, VulkanMaterial> _materials;
	std::unordered_map<std::string, VulkanMesh>		_meshes;

	// create material and add it to the map
	VulkanMaterial* create_material(
		VulkanRootSignature* RootSignature,
		VulkanPipelineState* PipelineState,
		const std::string&	 name)
	{
		VulkanMaterial mat;
		mat.RootSignature = RootSignature;
		mat.PipelineState = PipelineState;
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
	void draw_objects(VulkanCommandContext& Context, RenderObject* first, int count)
	{
		VulkanMesh*		lastMesh	 = nullptr;
		VulkanMaterial* lastMaterial = nullptr;
		for (int i = 0; i < count; i++)
		{
			RenderObject& object = first[i];

			// only bind the pipeline if it doesn't match with the already bound one
			if (object.material != lastMaterial)
			{
				Context.SetGraphicsPipelineState(object.material->PipelineState);
				Context.SetGraphicsRootSignature(object.material->RootSignature);
				Context.SetDescriptorSets();

				lastMaterial = object.material;
			}

			MeshPushConstants constants = {};
			XMStoreFloat4x4(&constants.Transform, XMMatrixTranspose(XMLoadFloat4x4(&object.transformMatrix)));
			constants.Id1 = TextureDescriptor1.Index;

			Context.SetPushConstants(sizeof(MeshPushConstants), &constants);

			// only bind the mesh if it's a different one from last bind
			if (object.mesh != lastMesh)
			{
				// bind the mesh vertex buffer with offset 0
				VkDeviceSize Offsets[]		 = { 0 };
				VkBuffer	 VertexBuffers[] = { object.mesh->VertexResource->As<VulkanBuffer>()->GetApiHandle() };
				vkCmdBindVertexBuffers(Context.CommandBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(
					Context.CommandBuffer,
					object.mesh->IndexResource->As<VulkanBuffer>()->GetApiHandle(),
					0,
					VK_INDEX_TYPE_UINT32);
				lastMesh = object.mesh;
			}
			Context.DrawIndexedInstanced(object.mesh->Mesh->Indices.size(), 1, 0, 0, 0);
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

#else

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
