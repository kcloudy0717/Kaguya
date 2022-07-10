#include "AssetImporter.h"
#include "AssetManager.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

DEFINE_LOG_CATEGORY(Asset);

namespace Asset
{
	static constexpr char AssetExtension[] = ".asset";

	MeshImporter::MeshImporter()
	{
		SupportedExtensions.insert(L".fbx");
		SupportedExtensions.insert(L".obj");
	}

	std::vector<AssetHandle> MeshImporter::Import(AssetManager* AssetManager, const MeshImportOptions& Options)
	{
		std::filesystem::path BinaryPath = Options.Path;
		BinaryPath.replace_extension(AssetExtension);

		std::vector<Mesh*> Meshes;
		if (exists(BinaryPath))
		{
			Meshes = ImportExisting(AssetManager, BinaryPath, Options);
		}
		else
		{
			const auto Path = Options.Path.string();

			Assimp::Importer   Importer;
			constexpr uint32_t ImporterFlags =
				aiProcess_ConvertToLeftHanded |
				aiProcess_JoinIdenticalVertices |
				aiProcess_Triangulate |
				aiProcess_SortByPType |
				aiProcess_GenNormals |
				aiProcess_GenUVCoords |
				aiProcess_OptimizeMeshes |
				aiProcess_ValidateDataStructure;
			const aiScene* paiScene = Importer.ReadFile(Path.data(), ImporterFlags);

			if (!paiScene || !paiScene->HasMeshes())
			{
				KAGUYA_LOG(Asset, Error, "{} error: {}", __FUNCTION__, Importer.GetErrorString());
				__debugbreak();
				return {};
			}

			Meshes.reserve(paiScene->mNumMeshes);
			for (unsigned m = 0; m < paiScene->mNumMeshes; ++m)
			{
				// Assimp object
				const aiMesh* paiMesh = paiScene->mMeshes[m];

				// Parse vertex data
				std::vector<Vertex> Vertices;
				Vertices.reserve(paiMesh->mNumVertices);
				for (unsigned int v = 0; v < paiMesh->mNumVertices; ++v)
				{
					Vertex& vertex = Vertices.emplace_back();
					// Position
					vertex.Position = { paiMesh->mVertices[v].x, paiMesh->mVertices[v].y, paiMesh->mVertices[v].z };

					// Texture coords
					if (paiMesh->HasTextureCoords(0))
					{
						vertex.TextureCoord = { paiMesh->mTextureCoords[0][v].x, paiMesh->mTextureCoords[0][v].y };
					}

					// Normal
					if (paiMesh->HasNormals())
					{
						vertex.Normal = { paiMesh->mNormals[v].x, paiMesh->mNormals[v].y, paiMesh->mNormals[v].z };
					}
				}

				DirectX::XMMATRIX Matrix = XMLoadFloat4x4(&Options.Matrix);
				XMVector3TransformCoordStream(
					&Vertices[0].Position,
					sizeof(Vertex),
					&Vertices[0].Position,
					sizeof(Vertex),
					Vertices.size(),
					Matrix);

				XMVector3TransformNormalStream(
					&Vertices[0].Normal,
					sizeof(Vertex),
					&Vertices[0].Normal,
					sizeof(Vertex),
					Vertices.size(),
					Matrix);

				// Parse index data
				std::vector<u32> Indices;
				Indices.reserve(static_cast<size_t>(paiMesh->mNumFaces) * 3);
				std::span Faces = { paiMesh->mFaces, paiMesh->mNumFaces };
				for (const auto& Face : Faces)
				{
					Indices.push_back(Face.mIndices[0]);
					Indices.push_back(Face.mIndices[1]);
					Indices.push_back(Face.mIndices[2]);
				}

				Mesh* Asset	   = Meshes.emplace_back(AssetManager->CreateAsset<Mesh>());
				Asset->Options = Options;
				if (paiMesh->mName.length == 0)
				{
					Asset->Name = Options.Path.filename().string();
				}
				else
				{
					Asset->Name = paiMesh->mName.C_Str();
				}

				Asset->Vertices = std::move(Vertices);
				Asset->Indices	= std::move(Indices);

				if (Options.GenerateMeshlets)
				{
					std::vector<DirectX::XMFLOAT3> Positions;
					Positions.reserve(Asset->Vertices.size());
					for (const auto& Vertex : Asset->Vertices)
					{
						Positions.emplace_back(Vertex.Position);
					}

					ComputeMeshlets(
						Asset->Indices.data(),
						Asset->Indices.size() / 3,
						Positions.data(),
						Positions.size(),
						nullptr,
						Asset->Meshlets,
						Asset->UniqueVertexIndices,
						Asset->PrimitiveIndices);
				}
			}

			Export(BinaryPath, Meshes);
		}

		std::vector<AssetHandle> Handles;
		Handles.reserve(Meshes.size());
		for (auto Mesh : Meshes)
		{
			Handles.push_back(Mesh->Handle);
			Mesh->UpdateInfo();
			Mesh->ComputeBoundingBox();
			AssetManager->RequestUpload(Mesh);
		}
		return Handles;
	}

	void MeshImporter::Export(const std::filesystem::path& BinaryPath, const std::vector<Mesh*>& Meshes)
	{
		FileStream	 Stream(BinaryPath, FileMode::Create, FileAccess::Write);
		BinaryWriter Writer(Stream);

		{
			ExportHeader Header = {};
			Header.NumMeshes	= Meshes.size();

			Writer.Write<ExportHeader>(Header);
		}
		for (const auto& Mesh : Meshes)
		{
			Writer.Write<size_t>(Mesh->Name.size());
			Writer.Write(Mesh->Name);

			MeshHeader Header			  = {};
			Header.NumVertices			  = Mesh->Vertices.size();
			Header.NumIndices			  = Mesh->Indices.size();
			Header.NumMeshlets			  = Mesh->Meshlets.size();
			Header.NumUniqueVertexIndices = Mesh->UniqueVertexIndices.size();
			Header.NumPrimitiveIndices	  = Mesh->PrimitiveIndices.size();

			Writer.Write<MeshHeader>(Header);
			Writer.Write(Mesh->Vertices.data(), Mesh->Vertices.size() * sizeof(Vertex));
			Writer.Write(Mesh->Indices.data(), Mesh->Indices.size() * sizeof(u32));
			Writer.Write(Mesh->Meshlets.data(), Mesh->Meshlets.size() * sizeof(DirectX::Meshlet));
			Writer.Write(Mesh->UniqueVertexIndices.data(), Mesh->UniqueVertexIndices.size() * sizeof(u8));
			Writer.Write(Mesh->PrimitiveIndices.data(), Mesh->PrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));
		}
	}

	std::vector<Mesh*> MeshImporter::ImportExisting(AssetManager* AssetManager, const std::filesystem::path& BinaryPath, const MeshImportOptions& Options)
	{
		std::vector<Mesh*> Meshes;

		FileStream	 Stream(BinaryPath, FileMode::Open, FileAccess::Read);
		BinaryReader Reader(Stream);

		{
			auto Header = Reader.Read<ExportHeader>();
			Meshes.resize(Header.NumMeshes);
		}
		for (auto& Asset : Meshes)
		{
			size_t		Length = Reader.Read<size_t>();
			std::string string;
			string.resize(Length);
			Reader.Read(string.data(), string.size() * sizeof(char));

			auto								  Header = Reader.Read<MeshHeader>();
			std::vector<Vertex>					  Vertices(Header.NumVertices);
			std::vector<u32>					  Indices(Header.NumIndices);
			std::vector<DirectX::Meshlet>		  Meshlets(Header.NumMeshlets);
			std::vector<u8>						  UniqueVertexIndices(Header.NumUniqueVertexIndices);
			std::vector<DirectX::MeshletTriangle> PrimitiveIndices(Header.NumPrimitiveIndices);

			Reader.Read(Vertices.data(), Vertices.size() * sizeof(Vertex));
			Reader.Read(Indices.data(), Indices.size() * sizeof(u32));
			Reader.Read(Meshlets.data(), Meshlets.size() * sizeof(DirectX::Meshlet));
			Reader.Read(UniqueVertexIndices.data(), UniqueVertexIndices.size() * sizeof(u8));
			Reader.Read(PrimitiveIndices.data(), PrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));

			Asset		   = AssetManager->CreateAsset<Mesh>();
			Asset->Options = Options;
			Asset->Name	   = string;

			Asset->SetVertices(std::move(Vertices));
			Asset->SetIndices(std::move(Indices));
			Asset->SetMeshlets(std::move(Meshlets));
			Asset->SetUniqueVertexIndices(std::move(UniqueVertexIndices));
			Asset->SetPrimitiveIndices(std::move(PrimitiveIndices));
		}
		return Meshes;
	}

	TextureImporter::TextureImporter()
	{
		SupportedExtensions.insert(L".dds");
		SupportedExtensions.insert(L".hdr");
		SupportedExtensions.insert(L".tga");
		SupportedExtensions.insert(L".bmp");
		SupportedExtensions.insert(L".png");
		SupportedExtensions.insert(L".gif");
		SupportedExtensions.insert(L".tiff");
		SupportedExtensions.insert(L".jpeg");
	}

	AssetHandle TextureImporter::Import(AssetManager* AssetManager, const TextureImportOptions& Options)
	{
		const auto& Path	  = Options.Path;
		const auto	Extension = Path.extension().string();

		DirectX::TexMetadata  TexMetadata = {};
		DirectX::ScratchImage OutImage	  = {};
		if (Extension == ".dds")
		{
			LoadFromDDSFile(Path.c_str(), DirectX::DDS_FLAGS::DDS_FLAGS_FORCE_RGB, &TexMetadata, OutImage);
		}
		else if (Extension == ".tga")
		{
			if (Options.GenerateMips)
			{
				DirectX::ScratchImage BaseImage;
				LoadFromTGAFile(Path.c_str(), &TexMetadata, BaseImage);
				GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, OutImage, false);
			}
			else
			{
				LoadFromTGAFile(Path.c_str(), &TexMetadata, OutImage);
			}
		}
		else if (Extension == ".hdr")
		{
			if (Options.GenerateMips)
			{
				DirectX::ScratchImage BaseImage;
				LoadFromHDRFile(Path.c_str(), &TexMetadata, BaseImage);
				GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, OutImage, false);
			}
			else
			{
				LoadFromHDRFile(Path.c_str(), &TexMetadata, OutImage);
			}
		}
		else
		{
			if (Options.GenerateMips)
			{
				DirectX::ScratchImage BaseImage;
				LoadFromWICFile(Path.c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, BaseImage);
				GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, OutImage, false);
			}
			else
			{
				LoadFromWICFile(Path.c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, OutImage);
			}
		}

		Texture* Asset	 = AssetManager->CreateAsset<Texture>();
		Asset->Options	 = Options;
		Asset->Extent	 = Math::Vec2i(static_cast<int>(TexMetadata.width), static_cast<int>(TexMetadata.height));
		Asset->IsCubemap = TexMetadata.IsCubemap();
		Asset->Name		 = Path.filename().string();
		Asset->TexImage	 = std::move(OutImage);
		AssetManager->RequestUpload(Asset);
		return Asset->Handle;
	}
} // namespace Asset
