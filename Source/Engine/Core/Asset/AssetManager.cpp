#include "AssetManager.h"

using namespace DirectX;

namespace Asset
{
	AssetManager::AssetManager(RHI::D3D12Device* Device)
		: Device(Device)
	{
		Thread = std::jthread(
			[=]()
			{
				RHI::D3D12LinkedDevice* LinkedDevice = Device->GetLinkedDevice();
				std::vector<Texture*>	Textures;

				while (true)
				{
					std::unique_lock Lock(Mutex);
					ConditionVariable.wait(Lock);

					if (Quit)
					{
						break;
					}

					Textures.clear();

					LinkedDevice->BeginResourceUpload();

					// Process Texture
					while (!TextureUploadQueue.empty())
					{
						Texture* Texture = TextureUploadQueue.front();
						TextureUploadQueue.pop();
						UploadTexture(Texture, LinkedDevice);
						Textures.push_back(Texture);
					}

					LinkedDevice->EndResourceUpload(true);

					for (auto Texture : Textures)
					{
						// Release memory
						Texture->Release();

						Texture->Handle.State = true;
						TextureRegistry.UpdateHandleState(Texture->Handle);
					}
				}
			});
	}

	AssetManager::~AssetManager()
	{
		Quit = true;
		ConditionVariable.notify_all();
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

	void AssetManager::UploadTexture(Texture* AssetTexture, RHI::D3D12LinkedDevice* Device)
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

		AssetTexture->DxTexture = RHI::D3D12Texture(Device, ResourceDesc, std::nullopt, AssetTexture->IsCubemap);
		AssetTexture->Srv		= RHI::D3D12ShaderResourceView(Device, &AssetTexture->DxTexture, false, std::nullopt, std::nullopt);

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

	void AssetManager::RequestUpload(Texture* Texture)
	{
		std::scoped_lock Lock(Mutex);
		TextureUploadQueue.push(Texture);
		ConditionVariable.notify_all();
	}

	void AssetManager::EnqueueRequest(Texture* Texture)
	{
		// TODO: This method doesn't work for some scenes, the HRESULT returned is 0x89240008 which
		// is not documented anywhere, it looks like is just segfault
		/*
			Exception thrown at 0x00007FFA9412466C in Kaguya.exe: Microsoft C++ exception: wil::ResultException at memory location 0x000000E889EFF7C0.
			Exception thrown at 0x00007FFA9412466C in Kaguya.exe: Microsoft C++ exception: [rethrow] at memory location 0x0000000000000000.
			Exception thrown at 0x00007FFA9412466C in Kaguya.exe: Microsoft C++ exception: RHI::D3D12Exception at memory location 0x000000E888CFD950.
			Unhandled exception at 0x00007FFA9412466C in Kaguya.exe: Microsoft C++ exception: RHI::D3D12Exception at memory location 0x000000E888CFD950.
		 */
		const auto& Metadata = Texture->TexImage.GetMetadata();

		DXGI_FORMAT Format = Metadata.format;
		if (Texture->Options.sRGB)
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

		Texture->DxTexture = RHI::D3D12Texture(Device->GetLinkedDevice(), ResourceDesc, std::nullopt, Texture->IsCubemap);
		Texture->Srv	   = RHI::D3D12ShaderResourceView(Device->GetLinkedDevice(), &Texture->DxTexture, false, std::nullopt, std::nullopt);

		const size_t NumSubresources = Metadata.mipLevels * Metadata.arraySize;
		auto		 Layouts		 = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * NumSubresources);
		auto		 NumRows		 = (UINT*)_alloca(sizeof(UINT) * NumSubresources);
		auto		 RowSizes		 = (UINT64*)_alloca(sizeof(UINT64) * NumSubresources);

		UINT64 IntermediateSize = 0;
		Device->GetD3D12Device()->GetCopyableFootprints(&ResourceDesc, 0, NumSubresources, 0, Layouts, NumRows, RowSizes, &IntermediateSize);

		// This Pixels pointer is a work around for underlying DirectX::Image::pixels pointer being aligned
		std::unique_ptr<u8[]> Pixels = std::make_unique<u8[]>(IntermediateSize);

		RHI::Arc<IDStorageStatusArray> StatusArray;
		Device->GetDStorageFactory()->CreateStatusArray(1, nullptr, IID_PPV_ARGS(&StatusArray));

		for (size_t ArraySlice = 0; ArraySlice < Metadata.arraySize; ++ArraySlice)
		{
			for (size_t MipSlice = 0; MipSlice < Metadata.mipLevels; ++MipSlice)
			{
				const size_t SubresourceIndex = MipSlice + (ArraySlice * Metadata.mipLevels);

				const auto&	 Layout			   = Layouts[SubresourceIndex];
				const UINT	 SubresourceHeight = NumRows[SubresourceIndex];
				const UINT64 UnpaddedRowPitch  = Layout.Footprint.RowPitch;
				u8*			 DstPixel		   = Pixels.get() + Layout.Offset;

				for (UINT z = 0; z < Layout.Footprint.Depth; ++z)
				{
					const DirectX::Image* Image = Texture->TexImage.GetImage(MipSlice, ArraySlice, z);
					assert(!!Image);
					const uint8_t* SrcPixel = Image->pixels;

					for (UINT y = 0; y < SubresourceHeight; ++y)
					{
						memcpy(DstPixel, SrcPixel, std::min(UnpaddedRowPitch, Image->rowPitch));
						DstPixel += UnpaddedRowPitch;
						SrcPixel += Image->rowPitch;
					}
				}
			}
		}

		DSTORAGE_REQUEST Request		= {};
		Request.Options.SourceType		= DSTORAGE_REQUEST_SOURCE_MEMORY;
		Request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;

		Request.Source.Memory.Source = Pixels.get();
		Request.Source.Memory.Size	 = IntermediateSize;

		Request.Destination.MultipleSubresources.Resource		  = Texture->DxTexture.GetResource();
		Request.Destination.MultipleSubresources.FirstSubresource = 0;

		Request.UncompressedSize = IntermediateSize;

		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);
		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueStatus(StatusArray.Get(), 0);

		RHI::D3D12SyncHandle Handle = Device->DStorageSubmit(DSTORAGE_REQUEST_SOURCE_MEMORY);
		Handle.WaitForCompletion();

		HRESULT HResult = StatusArray->GetHResult(0);
		VERIFY_D3D12_API(HResult);
		bool IsComplete = StatusArray->IsComplete(0);
		assert(IsComplete);

		// Check the status array for errors.
		// If an error was detected the first failure record
		// can be retrieved to get more details.
		DSTORAGE_ERROR_RECORD ErrorRecord = {};
		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->RetrieveErrorRecord(&ErrorRecord);
		VERIFY_D3D12_API(ErrorRecord.FirstFailure.HResult);

		// Release memory
		Texture->Release();

		Texture->Handle.State = true;
		TextureRegistry.UpdateHandleState(Texture->Handle);
	}

	void AssetManager::EnqueueRequest(Mesh* Mesh)
	{
		DSTORAGE_REQUEST Request		= {};
		Request.Options.SourceType		= DSTORAGE_REQUEST_SOURCE_MEMORY;
		Request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;

		UINT64 SizeInBytes = 0;

		//==================================================
		SizeInBytes			 = Mesh->Vertices.size() * sizeof(Vertex);
		Mesh->VertexResource = RHI::D3D12Buffer(Device->GetLinkedDevice(), SizeInBytes, sizeof(Vertex), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
		Mesh->VertexView	 = RHI::D3D12ShaderResourceView(Device->GetLinkedDevice(), &Mesh->VertexResource, true, 0, SizeInBytes);

		Request.Source.Memory.Source		= Mesh->Vertices.data();
		Request.Source.Memory.Size			= SizeInBytes;
		Request.Destination.Buffer.Resource = Mesh->VertexResource.GetResource();
		Request.Destination.Buffer.Offset	= 0;
		Request.Destination.Buffer.Size		= Request.Source.Memory.Size;
		Request.UncompressedSize			= SizeInBytes;
		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);

		//==================================================
		SizeInBytes			= Mesh->Indices.size() * sizeof(uint32_t);
		Mesh->IndexResource = RHI::D3D12Buffer(Device->GetLinkedDevice(), SizeInBytes, sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
		Mesh->IndexView		= RHI::D3D12ShaderResourceView(Device->GetLinkedDevice(), &Mesh->IndexResource, true, 0, SizeInBytes);

		Request.Source.Memory.Source		= Mesh->Indices.data();
		Request.Source.Memory.Size			= SizeInBytes;
		Request.Destination.Buffer.Resource = Mesh->IndexResource.GetResource();
		Request.Destination.Buffer.Offset	= 0;
		Request.Destination.Buffer.Size		= Request.Source.Memory.Size;
		Request.UncompressedSize			= SizeInBytes;
		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);

		if (Mesh->Options.GenerateMeshlets)
		{
			{
				//==================================================
				SizeInBytes			  = Mesh->Meshlets.size() * sizeof(Meshlet);
				Mesh->MeshletResource = RHI::D3D12Buffer(Device->GetLinkedDevice(), SizeInBytes, sizeof(Meshlet), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);

				Request.Source.Memory.Source		= Mesh->Meshlets.data();
				Request.Source.Memory.Size			= SizeInBytes;
				Request.Destination.Buffer.Resource = Mesh->MeshletResource.GetResource();
				Request.Destination.Buffer.Offset	= 0;
				Request.Destination.Buffer.Size		= Request.Source.Memory.Size;
				Request.UncompressedSize			= SizeInBytes;
				Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);
			}
			{
				//==================================================
				SizeInBytes						= Mesh->UniqueVertexIndices.size() * sizeof(uint8_t);
				Mesh->UniqueVertexIndexResource = RHI::D3D12Buffer(Device->GetLinkedDevice(), SizeInBytes, sizeof(uint8_t), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);

				Request.Source.Memory.Source		= Mesh->UniqueVertexIndices.data();
				Request.Source.Memory.Size			= SizeInBytes;
				Request.Destination.Buffer.Resource = Mesh->UniqueVertexIndexResource.GetResource();
				Request.Destination.Buffer.Offset	= 0;
				Request.Destination.Buffer.Size		= Request.Source.Memory.Size;
				Request.UncompressedSize			= SizeInBytes;
				Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);
			}
			{
				//==================================================
				SizeInBytes					 = Mesh->PrimitiveIndices.size() * sizeof(MeshletTriangle);
				Mesh->PrimitiveIndexResource = RHI::D3D12Buffer(Device->GetLinkedDevice(), SizeInBytes, sizeof(MeshletTriangle), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);

				Request.Source.Memory.Source		= Mesh->PrimitiveIndices.data();
				Request.Source.Memory.Size			= SizeInBytes;
				Request.Destination.Buffer.Resource = Mesh->PrimitiveIndexResource.GetResource();
				Request.Destination.Buffer.Offset	= 0;
				Request.Destination.Buffer.Size		= Request.Source.Memory.Size;
				Request.UncompressedSize			= SizeInBytes;
				Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->EnqueueRequest(&Request);
			}
		}

		RHI::D3D12SyncHandle Handle = Device->DStorageSubmit(DSTORAGE_REQUEST_SOURCE_MEMORY);
		Handle.WaitForCompletion();

		// Check the status array for errors.
		// If an error was detected the first failure record
		// can be retrieved to get more details.
		DSTORAGE_ERROR_RECORD ErrorRecord = {};
		Device->GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_MEMORY)->RetrieveErrorRecord(&ErrorRecord);
		VERIFY_D3D12_API(ErrorRecord.FirstFailure.HResult);

		D3D12_GPU_VIRTUAL_ADDRESS IndexAddress	= Mesh->IndexResource.GetGpuVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS VertexAddress = Mesh->VertexResource.GetGpuVirtualAddress();

		D3D12_RAYTRACING_GEOMETRY_DESC RaytracingGeometryDesc		= {};
		RaytracingGeometryDesc.Type									= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		RaytracingGeometryDesc.Flags								= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		RaytracingGeometryDesc.Triangles.Transform3x4				= NULL;
		RaytracingGeometryDesc.Triangles.IndexFormat				= DXGI_FORMAT_R32_UINT;
		RaytracingGeometryDesc.Triangles.VertexFormat				= DXGI_FORMAT_R32G32B32_FLOAT;
		RaytracingGeometryDesc.Triangles.IndexCount					= static_cast<UINT>(Mesh->Indices.size());
		RaytracingGeometryDesc.Triangles.VertexCount				= static_cast<UINT>(Mesh->Vertices.size());
		RaytracingGeometryDesc.Triangles.IndexBuffer				= IndexAddress;
		RaytracingGeometryDesc.Triangles.VertexBuffer.StartAddress	= VertexAddress;
		RaytracingGeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

		Mesh->Blas.AddGeometry(RaytracingGeometryDesc);

		// Release memory
		Mesh->Release();

		Mesh->Handle.State = true;
		MeshRegistry.UpdateHandleState(Mesh->Handle);
	}
} // namespace Asset
