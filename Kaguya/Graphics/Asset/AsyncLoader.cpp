#include "pch.h"
#include "AsyncLoader.h"

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

using namespace DirectX;

static Assimp::Importer	  s_Importer;
static constexpr uint32_t s_ImporterFlags =
	aiProcess_ConvertToLeftHanded | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_SortByPType |
	aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure;

AsyncImageLoader::TResourcePtr AsyncImageLoader::AsyncLoad(const TMetadata& Metadata)
{
	const auto start = std::chrono::high_resolution_clock::now();

	const auto& path	  = Metadata.Path;
	const auto	extension = path.extension().string();

	TexMetadata	 texMetadata  = {};
	ScratchImage scratchImage = {};
	if (extension == ".dds")
	{
		ThrowIfFailed(LoadFromDDSFile(path.c_str(), DDS_FLAGS::DDS_FLAGS_FORCE_RGB, &texMetadata, scratchImage));
	}
	else if (extension == ".tga")
	{
		ScratchImage baseImage;
		ThrowIfFailed(LoadFromTGAFile(path.c_str(), &texMetadata, baseImage));
		ThrowIfFailed(GenerateMipMaps(*baseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, scratchImage, false));
	}
	else if (extension == ".hdr")
	{
		ScratchImage baseImage;
		ThrowIfFailed(LoadFromHDRFile(path.c_str(), &texMetadata, baseImage));
		ThrowIfFailed(GenerateMipMaps(*baseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, scratchImage, false));
	}
	else
	{
		ScratchImage baseImage;
		ThrowIfFailed(LoadFromWICFile(path.c_str(), WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &texMetadata, baseImage));
		ThrowIfFailed(GenerateMipMaps(*baseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, scratchImage, false));
	}

	auto assetImage		   = std::make_shared<Asset::Image>();
	assetImage->Metadata   = Metadata;
	assetImage->Resolution = Vector2i(texMetadata.width, texMetadata.height);
	assetImage->Name	   = path.filename().string();
	assetImage->Image	   = std::move(scratchImage);

	const auto stop		= std::chrono::high_resolution_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	LOG_INFO("{} loaded in {}(ms)", path.string(), duration.count());

	return assetImage;
}

AsyncMeshLoader::TResourcePtr AsyncMeshLoader::AsyncLoad(const TMetadata& Metadata)
{
	const auto start = std::chrono::high_resolution_clock::now();

	const auto path = Metadata.Path.string();

	const aiScene* paiScene = s_Importer.ReadFile(path.data(), s_ImporterFlags);

	if (!paiScene || !paiScene->HasMeshes())
	{
		LOG_ERROR("Assimp::Importer error: {}", s_Importer.GetErrorString());
		return {};
	}

	auto assetMesh		= std::make_shared<Asset::Mesh>();
	assetMesh->Metadata = Metadata;
	assetMesh->Name		= Metadata.Path.filename().string();
	assetMesh->Submeshes.reserve(paiScene->mNumMeshes);

	uint32_t numVertices = 0;
	uint32_t numIndices	 = 0;
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
				vertex.Texture = { paiMesh->mTextureCoords[0][v].x, paiMesh->mTextureCoords[0][v].y };
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
		Asset::Submesh& assetSubmesh	= assetMesh->Submeshes.emplace_back();
		assetSubmesh.IndexCount			= indices.size();
		assetSubmesh.StartIndexLocation = numIndices;
		assetSubmesh.VertexCount		= vertices.size();
		assetSubmesh.BaseVertexLocation = numVertices;

		assetMesh->Vertices.insert(
			assetMesh->Vertices.end(),
			std::make_move_iterator(vertices.begin()),
			std::make_move_iterator(vertices.end()));
		assetMesh->Indices.insert(
			assetMesh->Indices.end(),
			std::make_move_iterator(indices.begin()),
			std::make_move_iterator(indices.end()));

		numIndices += indices.size();
		numVertices += vertices.size();
	}

	const auto stop		= std::chrono::high_resolution_clock::now();
	const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	LOG_INFO("{} loaded in {}(ms)", Metadata.Path.string(), duration.count());

	return assetMesh;
}
