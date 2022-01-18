#include "AssetManager.h"

using namespace DirectX;

void AssetManager::Initialize(D3D12Device* Device)
{
	Thread = std::jthread(
		[=]()
		{
			D3D12LinkedDevice* LinkedDevice = Device->GetDevice();

			while (true)
			{
				std::unique_lock Lock(Mutex);
				ConditionVariable.wait(Lock);

				if (Quit)
				{
					break;
				}

				std::vector<Mesh*>	  Meshes;
				std::vector<Texture*> Textures;

				LinkedDevice->BeginResourceUpload();

				// Process Mesh
				while (!MeshUploadQueue.empty())
				{
					Mesh* Mesh = MeshUploadQueue.front();
					MeshUploadQueue.pop();
					UploadMesh(Mesh, LinkedDevice);
					Meshes.push_back(Mesh);
				}

				// Process Texture
				while (!TextureUploadQueue.empty())
				{
					Texture* Texture = TextureUploadQueue.front();
					TextureUploadQueue.pop();
					UploadTexture(Texture, LinkedDevice);
					Textures.push_back(Texture);
				}

				LinkedDevice->EndResourceUpload(true);

				for (auto Mesh : Meshes)
				{
					// Release memory
					Mesh->Release();

					Mesh->Handle.State = true;
					MeshCache.UpdateHandleState(Mesh->Handle);
				}
				for (auto Texture : Textures)
				{
					// Release memory
					Texture->Release();

					Texture->Handle.State = true;
					TextureCache.UpdateHandleState(Texture->Handle);
				}
			}
		});
}

void AssetManager::Shutdown()
{
	Quit = true;
	ConditionVariable.notify_all();

	MeshCache.DestroyAll();
	TextureCache.DestroyAll();
}

AssetType AssetManager::GetAssetTypeFromExtension(const std::filesystem::path& Path)
{
	if (MeshImporter.SupportsExtension(Path))
	{
		return AssetType::Mesh;
	}
	if (TextureImporter.SupportsExtension(Path))
	{
		return AssetType::Texture;
	}

	return AssetType::Unknown;
}

void AssetManager::AsyncLoadImage(const TextureImportOptions& Options)
{
	TextureImporter.RequestAsyncLoad(Options);
}

void AssetManager::AsyncLoadMesh(const MeshImportOptions& Options)
{
	MeshImporter.RequestAsyncLoad(Options);
}

void AssetManager::UploadTexture(Texture* AssetTexture, D3D12LinkedDevice* Device)
{
	const auto& Metadata = AssetTexture->TexImage.GetMetadata();

	DXGI_FORMAT Format = Metadata.format;
	if (AssetTexture->Options.sRGB)
	{
		Format = DirectX::MakeSRGB(Format);
	}

	D3D12_RESOURCE_DESC ResourceDesc = {};
	switch (Metadata.dimension)
	{
	case TEX_DIMENSION_TEXTURE1D:
		ResourceDesc = CD3DX12_RESOURCE_DESC::Tex1D(
			Format,
			static_cast<UINT64>(Metadata.width),
			static_cast<UINT16>(Metadata.arraySize));
		break;

	case TEX_DIMENSION_TEXTURE2D:
		ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			Format,
			static_cast<UINT64>(Metadata.width),
			static_cast<UINT>(Metadata.height),
			static_cast<UINT16>(Metadata.arraySize),
			static_cast<UINT16>(Metadata.mipLevels));
		break;

	case TEX_DIMENSION_TEXTURE3D:
		ResourceDesc = CD3DX12_RESOURCE_DESC::Tex3D(
			Format,
			static_cast<UINT64>(Metadata.width),
			static_cast<UINT>(Metadata.height),
			static_cast<UINT16>(Metadata.depth));
		break;
	}

	AssetTexture->DxTexture = D3D12Texture(Device, ResourceDesc, std::nullopt, AssetTexture->IsCubemap);
	AssetTexture->SRV		= D3D12ShaderResourceView(Device, &AssetTexture->DxTexture, false, std::nullopt, std::nullopt);

	std::vector<D3D12_SUBRESOURCE_DATA> Subresources(AssetTexture->TexImage.GetImageCount());
	const auto							pImages = AssetTexture->TexImage.GetImages();
	for (size_t i = 0; i < AssetTexture->TexImage.GetImageCount(); ++i)
	{
		Subresources[i].RowPitch   = pImages[i].rowPitch;
		Subresources[i].SlicePitch = pImages[i].slicePitch;
		Subresources[i].pData	   = pImages[i].pixels;
	}

	Device->Upload(Subresources, AssetTexture->DxTexture.GetResource());
}

void AssetManager::UploadMesh(Mesh* AssetMesh, D3D12LinkedDevice* Device)
{
	UINT64 VertexBufferSizeInBytes			  = AssetMesh->Vertices.size() * sizeof(Vertex);
	UINT64 IndexBufferSizeInBytes			  = AssetMesh->Indices.size() * sizeof(uint32_t);
	UINT64 MeshletBufferSizeInBytes			  = AssetMesh->Meshlets.size() * sizeof(Meshlet);
	UINT64 UniqueVertexIndexBufferSizeInBytes = AssetMesh->UniqueVertexIndices.size() * sizeof(uint8_t);
	UINT64 PrimitiveIndexBufferSizeInBytes	  = AssetMesh->PrimitiveIndices.size() * sizeof(MeshletTriangle);

	auto VertexBuffer  = D3D12Buffer(Device, VertexBufferSizeInBytes, sizeof(Vertex), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	auto IndexBuffer   = D3D12Buffer(Device, IndexBufferSizeInBytes, sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	auto MeshletBuffer = D3D12Buffer(Device, MeshletBufferSizeInBytes, sizeof(Meshlet), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	auto UniqueVertexIndexBuffer = D3D12Buffer(Device, UniqueVertexIndexBufferSizeInBytes, sizeof(uint8_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	auto PrimitiveIndexBuffer = D3D12Buffer(Device, PrimitiveIndexBufferSizeInBytes, sizeof(MeshletTriangle), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Vertices.data();
		SubresourceData.RowPitch			   = VertexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = VertexBufferSizeInBytes;
		Device->Upload(SubresourceData, VertexBuffer.GetResource());
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Indices.data();
		SubresourceData.RowPitch			   = IndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = IndexBufferSizeInBytes;
		Device->Upload(SubresourceData, IndexBuffer.GetResource());
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Meshlets.data();
		SubresourceData.RowPitch			   = MeshletBufferSizeInBytes;
		SubresourceData.SlicePitch			   = MeshletBufferSizeInBytes;
		Device->Upload(SubresourceData, MeshletBuffer.GetResource());
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->UniqueVertexIndices.data();
		SubresourceData.RowPitch			   = UniqueVertexIndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = UniqueVertexIndexBufferSizeInBytes;
		Device->Upload(SubresourceData, UniqueVertexIndexBuffer.GetResource());
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->PrimitiveIndices.data();
		SubresourceData.RowPitch			   = PrimitiveIndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = PrimitiveIndexBufferSizeInBytes;
		Device->Upload(SubresourceData, PrimitiveIndexBuffer.GetResource());
	}

	AssetMesh->VertexResource			 = std::move(VertexBuffer);
	AssetMesh->IndexResource			 = std::move(IndexBuffer);
	AssetMesh->MeshletResource			 = std::move(MeshletBuffer);
	AssetMesh->UniqueVertexIndexResource = std::move(UniqueVertexIndexBuffer);
	AssetMesh->PrimitiveIndexResource	 = std::move(PrimitiveIndexBuffer);

	D3D12_GPU_VIRTUAL_ADDRESS IndexAddress	= AssetMesh->IndexResource.GetGpuVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS VertexAddress = AssetMesh->VertexResource.GetGpuVirtualAddress();

	D3D12_RAYTRACING_GEOMETRY_DESC RaytracingGeometryDesc		= {};
	RaytracingGeometryDesc.Type									= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	RaytracingGeometryDesc.Flags								= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	RaytracingGeometryDesc.Triangles.Transform3x4				= NULL;
	RaytracingGeometryDesc.Triangles.IndexFormat				= DXGI_FORMAT_R32_UINT;
	RaytracingGeometryDesc.Triangles.VertexFormat				= DXGI_FORMAT_R32G32B32_FLOAT;
	RaytracingGeometryDesc.Triangles.IndexCount					= static_cast<UINT>(AssetMesh->Indices.size());
	RaytracingGeometryDesc.Triangles.VertexCount				= static_cast<UINT>(AssetMesh->Vertices.size());
	RaytracingGeometryDesc.Triangles.IndexBuffer				= IndexAddress;
	RaytracingGeometryDesc.Triangles.VertexBuffer.StartAddress	= VertexAddress;
	RaytracingGeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

	AssetMesh->Blas.AddGeometry(RaytracingGeometryDesc);

	AssetMesh->VertexView = D3D12ShaderResourceView(Device, &AssetMesh->VertexResource, true, 0, VertexBufferSizeInBytes);
	AssetMesh->IndexView  = D3D12ShaderResourceView(Device, &AssetMesh->IndexResource, true, 0, IndexBufferSizeInBytes);
}

void AssetManager::RequestUpload(Texture* Texture)
{
	//D3D12LinkedDevice* Device = RenderCore::Device->GetDevice();
	//Device->BeginResourceUpload();
	//UploadTexture(Texture, Device);
	//Device->EndResourceUpload(true);
	//// Release memory
	//Texture->Release();

	//Texture->Handle.State = true;
	//TextureCache.UpdateHandleState(Texture->Handle);

	std::scoped_lock Lock(Mutex);
	TextureUploadQueue.push(std::move(Texture));
	ConditionVariable.notify_all();
}

void AssetManager::RequestUpload(Mesh* Mesh)
{
	//D3D12LinkedDevice* Device = RenderCore::Device->GetDevice();
	//Device->BeginResourceUpload();
	//UploadMesh(Mesh, Device);
	//Device->EndResourceUpload(true);
	//// Release memory
	//Mesh->Release();

	//Mesh->Handle.State = true;
	//MeshCache.UpdateHandleState(Mesh->Handle);

	std::scoped_lock Lock(Mutex);
	MeshUploadQueue.push(std::move(Mesh));
	ConditionVariable.notify_all();
}
