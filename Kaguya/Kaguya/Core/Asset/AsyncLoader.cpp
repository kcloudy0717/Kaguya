#include "AsyncLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

using namespace DirectX;

static Assimp::Importer	  s_Importer;
static constexpr uint32_t s_ImporterFlags =
	aiProcess_ConvertToLeftHanded | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_SortByPType |
	aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure;

AsyncImageLoader::TResourcePtr AsyncImageLoader::AsyncLoad(const Asset::ImageMetadata& Metadata)
{
	const auto start = std::chrono::high_resolution_clock::now();

	const auto& Path	  = Metadata.Path;
	const auto	Extension = Path.extension().string();

	TexMetadata	 TexMetadata = {};
	ScratchImage OutImage	 = {};
	if (Extension == ".dds")
	{
		assert(SUCCEEDED((LoadFromDDSFile(Path.c_str(), DDS_FLAGS::DDS_FLAGS_FORCE_RGB, &TexMetadata, OutImage))));
	}
	else if (Extension == ".tga")
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			assert(SUCCEEDED((LoadFromTGAFile(Path.c_str(), &TexMetadata, BaseImage))));
			assert(SUCCEEDED((GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false))));
		}
		else
		{
			assert(SUCCEEDED((LoadFromTGAFile(Path.c_str(), &TexMetadata, OutImage))));
		}
	}
	else if (Extension == ".hdr")
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			assert(SUCCEEDED((LoadFromHDRFile(Path.c_str(), &TexMetadata, BaseImage))));
			assert(SUCCEEDED((GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false))));
		}
		else
		{
			assert(SUCCEEDED((LoadFromHDRFile(Path.c_str(), &TexMetadata, OutImage))));
		}
	}
	else
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			assert(SUCCEEDED((LoadFromWICFile(Path.c_str(), WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, BaseImage))));
			assert(SUCCEEDED((GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false))));
		}
		else
		{
			assert(SUCCEEDED((LoadFromWICFile(Path.c_str(), WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, OutImage))));
		}
	}

	auto Image		  = std::make_shared<Asset::Image>();
	Image->Metadata	  = Metadata;
	Image->Resolution = Vector2i(static_cast<int>(TexMetadata.width), static_cast<int>(TexMetadata.height));
	Image->Name		  = Path.filename().string();
	Image->Image	  = std::move(OutImage);

	const auto stop		= std::chrono::high_resolution_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	LOG_INFO("{} loaded in {}(ms)", Path.string(), duration.count());

	return Image;
}

AsyncMeshLoader::TResourcePtr AsyncMeshLoader::AsyncLoad(const Asset::MeshMetadata& Metadata)
{
	const auto start = std::chrono::high_resolution_clock::now();

	const auto Path = Metadata.Path.string();

	const aiScene* paiScene = s_Importer.ReadFile(Path.data(), s_ImporterFlags);

	if (!paiScene || !paiScene->HasMeshes())
	{
		LOG_ERROR("Assimp::Importer error when loading {}", Path.data());
		LOG_ERROR("Error: {}", s_Importer.GetErrorString());
		return {};
	}

	auto Mesh	   = std::make_shared<Asset::Mesh>();
	Mesh->Metadata = Metadata;
	Mesh->Name	   = Metadata.Path.filename().string();
	Mesh->Submeshes.reserve(paiScene->mNumMeshes);

	uint32_t NumVertices = 0;
	uint32_t NumIndices	 = 0;
	for (size_t m = 0; m < paiScene->mNumMeshes; ++m)
	{
		const aiMesh* paiMesh = paiScene->mMeshes[m];

		// Parse vertex data
		std::vector<Vertex> vertices;
		vertices.reserve(paiMesh->mNumVertices);

		// Parse vertex data
		for (unsigned int v = 0; v < paiMesh->mNumVertices; ++v)
		{
			Vertex& vertex = vertices.emplace_back();
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

		// Parse index data
		std::vector<std::uint32_t> indices;
		indices.reserve(size_t(paiMesh->mNumFaces) * 3);
		for (unsigned int f = 0; f < paiMesh->mNumFaces; ++f)
		{
			const aiFace& aiFace = paiMesh->mFaces[f];

			indices.push_back(aiFace.mIndices[0]);
			indices.push_back(aiFace.mIndices[1]);
			indices.push_back(aiFace.mIndices[2]);
		}

		// Parse submesh indices
		Asset::Submesh& Submesh	   = Mesh->Submeshes.emplace_back();
		Submesh.IndexCount		   = static_cast<uint32_t>(indices.size());
		Submesh.StartIndexLocation = NumIndices;
		Submesh.VertexCount		   = static_cast<uint32_t>(vertices.size());
		Submesh.BaseVertexLocation = NumVertices;

		Mesh->Vertices.insert(
			Mesh->Vertices.end(),
			std::make_move_iterator(vertices.begin()),
			std::make_move_iterator(vertices.end()));
		Mesh->Indices.insert(
			Mesh->Indices.end(),
			std::make_move_iterator(indices.begin()),
			std::make_move_iterator(indices.end()));

		NumIndices += static_cast<uint32_t>(indices.size());
		NumVertices += static_cast<uint32_t>(vertices.size());
	}

	const auto stop		= std::chrono::high_resolution_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	LOG_INFO("{} loaded in {}(ms)", Metadata.Path.string(), duration.count());

	return Mesh;
}
