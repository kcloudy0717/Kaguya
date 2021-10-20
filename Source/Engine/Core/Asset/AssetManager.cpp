#include "AssetManager.h"
#include "RenderCore/RenderCore.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void AssetManager::Initialize()
{
	Thread = std::jthread(
		[&]()
		{
			D3D12LinkedDevice*	  Device = RenderCore::Device->GetDevice();
			D3D12ResourceUploader ResourceUploader(Device);

			while (true)
			{
				std::unique_lock _(Mutex);
				ConditionVariable.wait(_);

				if (Quit)
				{
					break;
				}

				std::vector<Mesh*>	  Meshes;
				std::vector<Texture*> Textures;

				ResourceUploader.Begin();

				// Process Mesh
				while (!MeshUploadQueue.empty())
				{
					Mesh* Mesh = MeshUploadQueue.front();
					MeshUploadQueue.pop();
					UploadMesh(Mesh, ResourceUploader);
					Meshes.push_back(Mesh);
				}

				// Process Texture
				while (!ImageUploadQueue.empty())
				{
					Texture* Texture = ImageUploadQueue.front();
					ImageUploadQueue.pop();
					UploadImage(Texture, ResourceUploader);
					Textures.push_back(Texture);
				}

				ResourceUploader.End(true);

				for (auto Mesh : Meshes)
				{
					Mesh->Handle.State = AssetState::Ready;
					MeshCache.UpdateHandleState(Mesh->Handle);
				}
				for (auto Texture : Textures)
				{
					Texture->Handle.State = AssetState::Ready;
					TextureCache.UpdateHandleState(Texture->Handle);
				}
			}
		});
}

void AssetManager::Shutdown()
{
	Quit = true;
	ConditionVariable.notify_all();

	Thread.join();

	TextureCache.DestroyAll();
	MeshCache.DestroyAll();
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
	if (!exists(Options.Path))
	{
		return;
	}
	TextureImporter.RequestAsyncLoad(Options);
}

void AssetManager::AsyncLoadMesh(const MeshImportOptions& Options)
{
	if (!exists(Options.Path))
	{
		return;
	}
	MeshImporter.RequestAsyncLoad(Options);
}

void AssetManager::UploadImage(Texture* AssetTexture, D3D12ResourceUploader& Uploader)
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

	AssetTexture->DxTexture = D3D12Texture(RenderCore::Device->GetDevice(), ResourceDesc, {});
	AssetTexture->SRV		= D3D12ShaderResourceView(RenderCore::Device->GetDevice());
	AssetTexture->DxTexture.CreateShaderResourceView(AssetTexture->SRV);

	std::vector<D3D12_SUBRESOURCE_DATA> Subresources(AssetTexture->TexImage.GetImageCount());
	const auto							pImages = AssetTexture->TexImage.GetImages();
	for (size_t i = 0; i < AssetTexture->TexImage.GetImageCount(); ++i)
	{
		Subresources[i].RowPitch   = pImages[i].rowPitch;
		Subresources[i].SlicePitch = pImages[i].slicePitch;
		Subresources[i].pData	   = pImages[i].pixels;
	}

	Uploader.Upload(Subresources, AssetTexture->DxTexture);
}

void AssetManager::UploadMesh(Mesh* AssetMesh, D3D12ResourceUploader& Uploader)
{
	D3D12LinkedDevice* Device = RenderCore::Device->GetDevice();

	UINT64 VertexBufferSizeInBytes			  = AssetMesh->Vertices.size() * sizeof(Vertex);
	UINT64 IndexBufferSizeInBytes			  = AssetMesh->Indices.size() * sizeof(uint32_t);
	UINT64 MeshletBufferSizeInBytes			  = AssetMesh->Meshlets.size() * sizeof(Meshlet);
	UINT64 UniqueVertexIndexBufferSizeInBytes = AssetMesh->UniqueVertexIndices.size() * sizeof(uint8_t);
	UINT64 PrimitiveIndexBufferSizeInBytes	  = AssetMesh->PrimitiveIndices.size() * sizeof(MeshletTriangle);

	auto VertexBuffer =
		D3D12Buffer(Device, VertexBufferSizeInBytes, sizeof(Vertex), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	auto IndexBuffer = D3D12Buffer(
		Device,
		IndexBufferSizeInBytes,
		sizeof(uint32_t),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_NONE);
	auto MeshletBuffer = D3D12Buffer(
		Device,
		MeshletBufferSizeInBytes,
		sizeof(Meshlet),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_NONE);
	auto UniqueVertexIndexBuffer = D3D12Buffer(
		Device,
		UniqueVertexIndexBufferSizeInBytes,
		sizeof(uint8_t),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_NONE);
	auto PrimitiveIndexBuffer = D3D12Buffer(
		Device,
		PrimitiveIndexBufferSizeInBytes,
		sizeof(MeshletTriangle),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_NONE);

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Vertices.data();
		SubresourceData.RowPitch			   = VertexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = VertexBufferSizeInBytes;
		Uploader.Upload(SubresourceData, VertexBuffer);
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Indices.data();
		SubresourceData.RowPitch			   = IndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = IndexBufferSizeInBytes;
		Uploader.Upload(SubresourceData, IndexBuffer);
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->Meshlets.data();
		SubresourceData.RowPitch			   = MeshletBufferSizeInBytes;
		SubresourceData.SlicePitch			   = MeshletBufferSizeInBytes;
		Uploader.Upload(SubresourceData, MeshletBuffer);
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->UniqueVertexIndices.data();
		SubresourceData.RowPitch			   = UniqueVertexIndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = UniqueVertexIndexBufferSizeInBytes;
		Uploader.Upload(SubresourceData, UniqueVertexIndexBuffer);
	}

	{
		D3D12_SUBRESOURCE_DATA SubresourceData = {};
		SubresourceData.pData				   = AssetMesh->PrimitiveIndices.data();
		SubresourceData.RowPitch			   = PrimitiveIndexBufferSizeInBytes;
		SubresourceData.SlicePitch			   = PrimitiveIndexBufferSizeInBytes;
		Uploader.Upload(SubresourceData, PrimitiveIndexBuffer);
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
}
