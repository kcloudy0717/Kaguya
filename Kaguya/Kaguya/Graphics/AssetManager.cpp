#include "AssetManager.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void AssetManager::Initialize()
{
	AsyncImageLoader.SetDelegate(
		[&](auto pImage)
		{
			std::scoped_lock _(Mutex);

			ImageUploadQueue.push(std::move(pImage));
			ConditionVariable.notify_all();
		});

	AsyncMeshLoader.SetDelegate(
		[&](auto pMesh)
		{
			std::scoped_lock _(Mutex);

			MeshUploadQueue.push(std::move(pMesh));
			ConditionVariable.notify_all();
		});

	struct threadwrapper
	{
		static unsigned int WINAPI thunk(LPVOID lpParameter)
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

				ResourceUploader.Begin();

				// container to store assets after it has been uploaded to be added to the AssetCache
				std::vector<std::shared_ptr<Asset::Image>> UploadedImages;
				std::vector<std::shared_ptr<Asset::Mesh>>  UploadedMeshes;

				// Process Image
				while (!ImageUploadQueue.empty())
				{
					std::shared_ptr<Asset::Image> AssetImage = ImageUploadQueue.front();
					ImageUploadQueue.pop();

					const auto& Image	 = AssetImage->Image;
					const auto& Metadata = Image.GetMetadata();

					DXGI_FORMAT Format = Metadata.format;
					if (AssetImage->Metadata.sRGB)
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

					AssetImage->Texture = D3D12Texture(RenderCore::Device->GetDevice(), ResourceDesc, {});
					AssetImage->SRV		= D3D12ShaderResourceView(RenderCore::Device->GetDevice());
					AssetImage->Texture.CreateShaderResourceView(AssetImage->SRV);

					std::vector<D3D12_SUBRESOURCE_DATA> Subresources(Image.GetImageCount());
					const auto							pImages = Image.GetImages();
					for (size_t i = 0; i < Image.GetImageCount(); ++i)
					{
						Subresources[i].RowPitch   = pImages[i].rowPitch;
						Subresources[i].SlicePitch = pImages[i].slicePitch;
						Subresources[i].pData	   = pImages[i].pixels;
					}

					ResourceUploader.Upload(Subresources, AssetImage->Texture);

					UploadedImages.push_back(AssetImage);
				}

				// Process Mesh
				while (!MeshUploadQueue.empty())
				{
					std::shared_ptr<Asset::Mesh> AssetMesh = MeshUploadQueue.front();
					MeshUploadQueue.pop();

					UINT64 VertexBufferSizeInBytes = AssetMesh->Vertices.size() * sizeof(Vertex);
					UINT64 IndexBufferSizeInBytes  = AssetMesh->Indices.size() * sizeof(unsigned int);

					auto VertexBuffer = D3D12Buffer(
						Device,
						VertexBufferSizeInBytes,
						sizeof(Vertex),
						D3D12_HEAP_TYPE_DEFAULT,
						D3D12_RESOURCE_FLAG_NONE);
					auto IndexBuffer = D3D12Buffer(
						Device,
						IndexBufferSizeInBytes,
						sizeof(unsigned int),
						D3D12_HEAP_TYPE_DEFAULT,
						D3D12_RESOURCE_FLAG_NONE);

					// Upload vertex data
					{
						D3D12_SUBRESOURCE_DATA SubresourceData = {};
						SubresourceData.pData				   = AssetMesh->Vertices.data();
						SubresourceData.RowPitch			   = VertexBufferSizeInBytes;
						SubresourceData.SlicePitch			   = VertexBufferSizeInBytes;
						ResourceUploader.Upload(SubresourceData, VertexBuffer);
					}

					// Upload index data
					{
						D3D12_SUBRESOURCE_DATA SubresourceData = {};
						SubresourceData.pData				   = AssetMesh->Indices.data();
						SubresourceData.RowPitch			   = IndexBufferSizeInBytes;
						SubresourceData.SlicePitch			   = IndexBufferSizeInBytes;
						ResourceUploader.Upload(SubresourceData, IndexBuffer);
					}

					AssetMesh->VertexResource = std::move(VertexBuffer);
					AssetMesh->IndexResource  = std::move(IndexBuffer);

					for (const auto& Submesh : AssetMesh->Submeshes)
					{
						D3D12_GPU_VIRTUAL_ADDRESS IndexAddress = AssetMesh->IndexResource.GetGpuVirtualAddress() +
																 Submesh.StartIndexLocation * sizeof(unsigned int);
						D3D12_GPU_VIRTUAL_ADDRESS VertexAddress = AssetMesh->VertexResource.GetGpuVirtualAddress() +
																  Submesh.BaseVertexLocation * sizeof(Vertex);

						D3D12_RAYTRACING_GEOMETRY_DESC RaytracingGeometryDesc = {};
						RaytracingGeometryDesc.Type					  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
						RaytracingGeometryDesc.Flags				  = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
						RaytracingGeometryDesc.Triangles.Transform3x4 = NULL;
						RaytracingGeometryDesc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
						RaytracingGeometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
						RaytracingGeometryDesc.Triangles.IndexCount	  = Submesh.IndexCount;
						RaytracingGeometryDesc.Triangles.VertexCount  = Submesh.VertexCount;
						RaytracingGeometryDesc.Triangles.IndexBuffer  = IndexAddress;
						RaytracingGeometryDesc.Triangles.VertexBuffer.StartAddress	= VertexAddress;
						RaytracingGeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

						AssetMesh->Blas.AddGeometry(RaytracingGeometryDesc);
					}

					UploadedMeshes.push_back(AssetMesh);
				}

				ResourceUploader.End(true);

				for (auto& Image : UploadedImages)
				{
					std::string String = Image->Metadata.Path.string();
					UINT64		Hash   = CityHash64(String.data(), String.size());
					ImageCache.Emplace(Hash, Image);
				}

				for (auto& Mesh : UploadedMeshes)
				{
					std::string String = Mesh->Metadata.Path.string();
					UINT64		Hash   = CityHash64(String.data(), String.size());

					MeshCache.Emplace(Hash, Mesh);
				}
			}

			return EXIT_SUCCESS;
		}
	};

	Thread.reset(reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, threadwrapper::thunk, nullptr, 0, nullptr)));
}

void AssetManager::Shutdown()
{
	Quit = true;
	ConditionVariable.notify_all();

	::WaitForSingleObject(Thread.get(), INFINITE);

	ImageCache.DestroyAll();
	MeshCache.DestroyAll();
}

void AssetManager::AsyncLoadImage(const std::filesystem::path& Path, bool sRGB)
{
	if (!std::filesystem::exists(Path))
	{
		return;
	}

	std::string String = Path.string();
	UINT64		Hash   = CityHash64(String.data(), String.size());
	if (ImageCache.Exist(Hash))
	{
		LOG_INFO("{} Exists", String);
		return;
	}

	Asset::ImageMetadata Metadata = { .Path = Path, .sRGB = sRGB };
	AsyncImageLoader.RequestAsyncLoad(1, &Metadata);
}

void AssetManager::AsyncLoadMesh(const std::filesystem::path& Path, bool KeepGeometryInRAM)
{
	if (!std::filesystem::exists(Path))
	{
		return;
	}

	std::string String = Path.string();
	UINT64		Hash   = CityHash64(String.data(), String.size());
	if (MeshCache.Exist(Hash))
	{
		LOG_INFO("{} Exists", String);
		return;
	}

	Asset::MeshMetadata Metadata = { .Path = Path, .KeepGeometryInRAM = KeepGeometryInRAM };
	AsyncMeshLoader.RequestAsyncLoad(1, &Metadata);
}
