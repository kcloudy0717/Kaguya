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

void StreamWrite(std::ostream& os, const SpvReflectDescriptorBinding& obj, bool write_set, const char* indent = "")
{
	const char* t = indent;

	os << " " << obj.name;
	os << "\n";
	os << t;
	os << ToStringDescriptorType(obj.descriptor_type);
	os << " "
	   << "(" << ToStringResourceType(obj.resource_type) << ")";
	if ((obj.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
		(obj.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
		(obj.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
		(obj.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER))
	{
		os << "\n";
		os << t;
		os << "dim=" << obj.image.dim << ", ";
		os << "depth=" << obj.image.depth << ", ";
		os << "arrayed=" << obj.image.arrayed << ", ";
		os << "ms=" << obj.image.ms << ", ";
		os << "sampled=" << obj.image.sampled;
	}
}

void StreamWrite(std::ostream& os, const SpvReflectShaderModule& obj, const char* indent = "")
{
	os << "entry point     : " << obj.entry_point_name << "\n";
	os << "source lang     : " << spvReflectSourceLanguage(obj.source_language) << "\n";
	os << "source lang ver : " << obj.source_language_version;
}

void StreamWrite(std::ostream& os, const SpvReflectDescriptorBinding& obj)
{
	StreamWrite(os, obj, true, "  ");
}

// Specialized stream-writer that only includes descriptor bindings.
void StreamWrite(std::ostream& os, const spv_reflect::ShaderModule& obj)
{
	const char* t	  = "  ";
	const char* tt	  = "    ";
	const char* ttt	  = "      ";
	const char* tttt  = "        ";
	const char* ttttt = "          ";

	StreamWrite(os, obj.GetShaderModule(), "");

	SpvReflectResult						  result = SPV_REFLECT_RESULT_NOT_READY;
	uint32_t								  count	 = 0;
	std::vector<SpvReflectInterfaceVariable*> variables;
	std::vector<SpvReflectDescriptorBinding*> bindings;
	std::vector<SpvReflectDescriptorSet*>	  sets;

	count  = 0;
	result = obj.EnumerateDescriptorBindings(&count, nullptr);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
	bindings.resize(count);
	result = obj.EnumerateDescriptorBindings(&count, bindings.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);
	if (count > 0)
	{
		os << "\n";
		os << "\n";
		os << t << "Descriptor bindings: " << count << "\n";
		for (size_t i = 0; i < bindings.size(); ++i)
		{
			SpvReflectDescriptorBinding* p_binding = bindings[i];
			assert(result == SPV_REFLECT_RESULT_SUCCESS);
			os << tt << i << ":";
			StreamWrite(os, *p_binding, true, ttt);
			if (i < (count - 1))
			{
				os << "\n\n";
			}
		}
	}
}

class VulkanEngine final : public Application
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

		// The size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize PoolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
											 { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
											 { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
											 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
											 { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
											 { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 } };

		auto DescriptorPoolCreateInfo		   = VkStruct<VkDescriptorPoolCreateInfo>();
		DescriptorPoolCreateInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescriptorPoolCreateInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		DescriptorPoolCreateInfo.maxSets	   = 1024;
		DescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(std::size(PoolSizes));
		DescriptorPoolCreateInfo.pPoolSizes	   = PoolSizes;

		VERIFY_VULKAN_API(
			vkCreateDescriptorPool(Device.GetVkDevice(), &DescriptorPoolCreateInfo, nullptr, &ImguiDescriptorPool));

		ImGui_ImplVulkan_InitInfo ImGuiVulkan = {};
		ImGuiVulkan.Instance				  = Device.GetVkInstance();
		ImGuiVulkan.PhysicalDevice			  = Device.GetVkPhysicalDevice();
		ImGuiVulkan.Device					  = Device.GetVkDevice();
		ImGuiVulkan.Queue					  = Device.GetGraphicsQueue().GetApiHandle();
		ImGuiVulkan.DescriptorPool			  = ImguiDescriptorPool;
		ImGuiVulkan.MinImageCount			  = 3;
		ImGuiVulkan.ImageCount				  = 3;
		ImGuiVulkan.MSAASamples				  = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&ImGuiVulkan, RenderPass->As<VulkanRenderPass>()->GetApiHandle());
		GraphicsImmediate(
			[&](VkCommandBuffer CommandBuffer)
			{
				ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer);
			});
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		LoadMeshes();
		InitScene();

		return true;
	}

	void Shutdown() override
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(Device.GetVkDevice(), ImguiDescriptorPool, nullptr);

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

		Context.BeginRenderPass(RenderPass.Get(), RenderTargets[swapchainImageIndex].Get());

		RHIViewport Viewport	= RHIViewport((float)SwapChain.GetWidth(), (float)SwapChain.GetHeight());
		RHIRect		ScissorRect = { 0, 0, SwapChain.GetWidth(), SwapChain.GetHeight() };
		Context.SetViewports(1, &Viewport, &ScissorRect);

		DrawObjects(Context, Renderables.size(), Renderables.data());

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
		Desc.AddRenderTarget({ .Format	   = SwapChain.GetRHIFormat(),
							   .LoadOp	   = ELoadOp::Clear,
							   .StoreOp	   = EStoreOp::Store,
							   .ClearValue = ClearValue(SwapChain.GetRHIFormat(), Color) });
		Desc.SetDepthStencil({ .Format	   = ERHIFormat::D32,
							   .LoadOp	   = ELoadOp::Clear,
							   .StoreOp	   = EStoreOp::Store,
							   .ClearValue = ClearValue(ERHIFormat::D32, 1.0f, 0xFF) });

		RenderPass = Device.CreateRenderPass(Desc);
	}

	void InitFrameBuffers()
	{
		const uint32_t ImageCount = SwapChain.GetImageCount();
		RenderTargets.resize(ImageCount);
		for (uint32_t i = 0; i < ImageCount; i++)
		{
			RenderTargetDesc Desc = {};
			Desc.RenderPass		  = RenderPass.Get();
			Desc.Width			  = SwapChain.GetWidth();
			Desc.Height			  = SwapChain.GetHeight();
			Desc.AddRenderTarget(SwapChain.GetBackbuffer(i));
			Desc.DepthStencil = DepthBuffer.Get();
			RenderTargets[i]  = Device.CreateRenderTarget(Desc);
		}
	}

	void InitDescriptors()
	{
		{
			RootSignatureDesc Desc = {};
			Desc.AddPushConstant<MeshPushConstants>();
			Desc.AddRootConstantBufferView(0);
			RootSignature = Device.CreateRootSignature(Desc);
		}
		//{
		//	VulkanDescriptorHandle Handle =
		//		Device.GetResourceDescriptorHeap().AllocateDescriptorHandle(EDescriptorType::ConstantBuffer);
		//	Handle.Resource = SceneConstants.Get();
		//
		//	Device.GetResourceDescriptorHeap().UpdateDescriptor(Handle);
		//}
		{
			DescriptorHandle Handle = Device.AllocateSampler();
			SamplerDesc		 SamplerDesc(
				 ESamplerFilter::Point,
				 ESamplerAddressMode::Wrap,
				 ESamplerAddressMode::Wrap,
				 ESamplerAddressMode::Wrap,
				 0.0f,
				 0,
				 ComparisonFunc::Never,
				 0.0f,
				 0.0f,
				 0.0f);

			Device.CreateSampler(SamplerDesc, Handle);
		}
	}

	void InitPipelines()
	{
		spv_reflect::ShaderModule VSModule;
		Shader					  VS = ShaderCompiler.SpirVCodeGen(
			   Device.GetVkDevice(),
			   EShaderType::Vertex,
			   ExecutableDirectory / "Shaders/Vulkan/Triangle.vs.hlsl",
			   L"main",
			   {});
		VSModule = spv_reflect::ShaderModule(VS.GetBufferSize(), VS.GetBufferPointer());
		if (VSModule.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
		{
			LOG_ERROR("Error");
		}

		StreamWrite(std::cout, VSModule);

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

		CreateMaterial(RootSignature->As<VulkanRootSignature>(), MeshPipeline->As<VulkanPipelineState>(), "Default");
	}

	void LoadMeshes()
	{
		Asset::MeshMetadata MeshMetadata = {};
		MeshMetadata.Path				 = ExecutableDirectory / "Assets/models/Sphere.obj";
		SphereMesh.Mesh					 = MeshLoader.AsyncLoad(MeshMetadata);

		UploadMesh(SphereMesh);

		LoadImageFromPath("Assets/models/bedroom/textures/Teapot.tga");

		SRV							= Device.AllocateShaderResourceView();
		ShaderResourceViewDesc Desc = {};
		Device.CreateShaderResourceView(Texture.Get(), Desc, SRV);
	}

	void InitScene()
	{
		RenderObject monkey = {};
		monkey.mesh			= &SphereMesh;
		monkey.material		= GetMaterial("Default");
		XMStoreFloat4x4(&monkey.transformMatrix, DirectX::XMMatrixIdentity());

		Renderables.push_back(monkey);

		for (int x = -20; x <= 20; x++)
		{
			for (int y = -20; y <= 20; y++)
			{
				RenderObject tri = {};
				tri.mesh		 = &SphereMesh;
				tri.material	 = GetMaterial("Default");
				XMStoreFloat4x4(
					&tri.transformMatrix,
					DirectX::XMMatrixTranslation(float(x) * 2.5f, 0, float(y) * 2.5f));

				Renderables.push_back(tri);
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

	// create material and add it to the map
	VulkanMaterial* CreateMaterial(
		VulkanRootSignature* RootSignature,
		VulkanPipelineState* PipelineState,
		const std::string&	 Name)
	{
		VulkanMaterial Material = {};
		Material.RootSignature	= RootSignature;
		Material.PipelineState	= PipelineState;
		Materials[Name]			= std::move(Material);
		return &Materials[Name];
	}

	VulkanMaterial* GetMaterial(const std::string& Name)
	{
		if (auto iter = Materials.find(Name); iter != Materials.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	VulkanMesh* GetMesh(const std::string& name)
	{
		if (auto iter = Meshes.find(name); iter != Meshes.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	void DrawObjects(VulkanCommandContext& Context, int NumRenderObjects, RenderObject* pRenderObjects)
	{
		VulkanMesh*		lastMesh	 = nullptr;
		VulkanMaterial* lastMaterial = nullptr;
		for (int i = 0; i < NumRenderObjects; i++)
		{
			RenderObject& object = pRenderObjects[i];

			// only bind the pipeline if it doesn't match with the already bound one
			if (object.material != lastMaterial)
			{
				Context.SetGraphicsPipelineState(object.material->PipelineState);
				Context.SetGraphicsRootSignature(object.material->RootSignature);
				Context.SetDescriptorSets();
				VkDescriptorBufferInfo descriptor = {};
				descriptor.buffer				  = SceneConstants->As<VulkanBuffer>()->GetApiHandle();
				descriptor.offset				  = 0;
				descriptor.range				  = SceneConstants->As<VulkanBuffer>()->GetDesc().size;

				auto writeDescriptorSet			   = VkStruct<VkWriteDescriptorSet>();
				writeDescriptorSet.dstSet		   = 0;
				writeDescriptorSet.dstBinding	   = 0;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.pBufferInfo	   = &descriptor;

				VulkanAPI::vkCmdPushDescriptorSetKHR(
					Context.CommandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					RootSignature->As<VulkanRootSignature>()->GetApiHandle(),
					2,
					1,
					&writeDescriptorSet);

				lastMaterial = object.material;
			}

			MeshPushConstants constants = {};
			XMStoreFloat4x4(&constants.Transform, XMMatrixTranspose(XMLoadFloat4x4(&object.transformMatrix)));
			constants.Id1 = SRV.Index;

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

private:
	ShaderCompiler ShaderCompiler;

	VulkanDevice	 Device;
	VkDescriptorPool ImguiDescriptorPool = nullptr;

	VulkanSwapChain SwapChain;

	RefPtr<IRHIRenderPass>				  RenderPass;
	std::vector<RefPtr<IRHIRenderTarget>> RenderTargets;

	RefPtr<IRHIRootSignature> RootSignature;
	RefPtr<IRHIPipelineState> MeshPipeline;
	VulkanMesh				  SphereMesh;

	RefPtr<IRHITexture> DepthBuffer;

	RefPtr<IRHIBuffer>	SceneConstants;
	RefPtr<IRHITexture> Texture;
	DescriptorHandle	SRV;

	World			 World;
	AsyncMeshLoader	 MeshLoader;
	AsyncImageLoader ImageLoader;

	// default array of renderable objects
	std::vector<RenderObject> Renderables;

	std::unordered_map<std::string, VulkanMaterial> Materials;
	std::unordered_map<std::string, VulkanMesh>		Meshes;
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
