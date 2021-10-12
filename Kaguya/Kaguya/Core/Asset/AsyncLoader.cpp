#include "AsyncLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

static Assimp::Importer	  s_Importer;
static constexpr uint32_t s_ImporterFlags =
	aiProcess_ConvertToLeftHanded | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_SortByPType |
	aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure;

template<typename Resolution = std::chrono::milliseconds>
class ExecutionTimer
{
public:
	using Clock = std::conditional_t<
		std::chrono::high_resolution_clock::is_steady,
		std::chrono::high_resolution_clock,
		std::chrono::steady_clock>;

	template<typename TLambda>
	ExecutionTimer(TLambda Message)
		: Message(Message)
	{
	}

	~ExecutionTimer() { Message(std::chrono::duration_cast<Resolution>(Clock::now() - Start)); }

private:
	Clock::time_point				   Start = Clock::now();
	Delegate<void(Resolution Elapsed)> Message;
};

AsyncImageLoader::TResourcePtr AsyncImageLoader::AsyncLoad(const Asset::ImageMetadata& Metadata)
{
	const auto& Path	  = Metadata.Path;
	const auto	Extension = Path.extension().string();

	ExecutionTimer Timer(
		[&](auto Elapsed)
		{
			LOG_INFO("{} loaded in {}(ms)", Path.string(), Elapsed.count());
		});

	TexMetadata	 TexMetadata = {};
	ScratchImage OutImage	 = {};
	if (Extension == ".dds")
	{
		LoadFromDDSFile(Path.c_str(), DDS_FLAGS::DDS_FLAGS_FORCE_RGB, &TexMetadata, OutImage);
	}
	else if (Extension == ".tga")
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			LoadFromTGAFile(Path.c_str(), &TexMetadata, BaseImage);
			GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false);
		}
		else
		{
			LoadFromTGAFile(Path.c_str(), &TexMetadata, OutImage);
		}
	}
	else if (Extension == ".hdr")
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			LoadFromHDRFile(Path.c_str(), &TexMetadata, BaseImage);
			GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false);
		}
		else
		{
			LoadFromHDRFile(Path.c_str(), &TexMetadata, OutImage);
		}
	}
	else
	{
		if (Metadata.GenerateMips)
		{
			ScratchImage BaseImage;
			LoadFromWICFile(Path.c_str(), WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, BaseImage);
			GenerateMipMaps(*BaseImage.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, OutImage, false);
		}
		else
		{
			LoadFromWICFile(Path.c_str(), WIC_FLAGS::WIC_FLAGS_FORCE_RGB, &TexMetadata, OutImage);
		}
	}

	auto Image		  = std::make_shared<Asset::Image>();
	Image->Metadata	  = Metadata;
	Image->Resolution = Vector2i(static_cast<int>(TexMetadata.width), static_cast<int>(TexMetadata.height));
	Image->Name		  = Path.filename().string();
	Image->Image	  = std::move(OutImage);

	return Image;
}

AsyncMeshLoader::TResourcePtr AsyncMeshLoader::AsyncLoad(const Asset::MeshMetadata& Metadata)
{
	ExecutionTimer Timer(
		[&](auto Elapsed)
		{
			LOG_INFO("{} loaded in {}(ms)", Metadata.Path.string(), Elapsed.count());
		});

	TResourcePtr Mesh;

	std::filesystem::path BinaryPath = Metadata.Path;
	BinaryPath.replace_extension("kaguya.mesh.bin");

	if (std::filesystem::exists(BinaryPath))
	{
		Mesh = ImportMesh(BinaryPath, Metadata);
	}
	else
	{
		Mesh		   = std::make_shared<Asset::Mesh>();
		Mesh->Metadata = Metadata;
		Mesh->Name	   = Metadata.Path.filename().string();

		const auto Path = Metadata.Path.string();

		const aiScene* paiScene = s_Importer.ReadFile(Path.data(), s_ImporterFlags);

		if (!paiScene || !paiScene->HasMeshes())
		{
			LOG_ERROR("Assimp::Importer error when loading {}", Path.data());
			LOG_ERROR("Error: {}", s_Importer.GetErrorString());
			return {};
		}

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
			indices.reserve(static_cast<size_t>(paiMesh->mNumFaces) * 3);
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

		std::vector<XMFLOAT3> Positions;
		Positions.reserve(Mesh->Vertices.size());
		for (const auto& Vertex : Mesh->Vertices)
		{
			Positions.emplace_back(Vertex.Position);
		}

		ComputeMeshlets(
			Mesh->Indices.data(),
			Mesh->Indices.size() / 3,
			Positions.data(),
			Positions.size(),
			nullptr,
			Mesh->Meshlets,
			Mesh->UniqueVertexIndices,
			Mesh->PrimitiveIndices);

		ExportMesh(BinaryPath, Mesh);
	}

	Mesh->ComputeBoundingBox();

	return Mesh;
}

void AsyncMeshLoader::ExportMesh(const std::filesystem::path& BinaryPath, TResourcePtr Mesh)
{
	MeshHeader Header			  = {};
	Header.NumVertices			  = Mesh->Vertices.size();
	Header.NumIndices			  = Mesh->Indices.size();
	Header.NumMeshlets			  = Mesh->Meshlets.size();
	Header.NumUniqueVertexIndices = Mesh->UniqueVertexIndices.size();
	Header.NumPrimitiveIndices	  = Mesh->PrimitiveIndices.size();
	Header.NumSubmeshes			  = Mesh->Submeshes.size();

	FileStream	 Stream(BinaryPath, FileMode::Create, FileAccess::Write);
	BinaryWriter Writer(Stream);

	Writer.Write<MeshHeader>(Header);
	Writer.Write(Mesh->Vertices.data(), Mesh->Vertices.size() * sizeof(Vertex));
	Writer.Write(Mesh->Indices.data(), Mesh->Indices.size() * sizeof(std::uint32_t));
	Writer.Write(Mesh->Meshlets.data(), Mesh->Meshlets.size() * sizeof(Meshlet));
	Writer.Write(Mesh->UniqueVertexIndices.data(), Mesh->UniqueVertexIndices.size() * sizeof(uint8_t));
	Writer.Write(Mesh->PrimitiveIndices.data(), Mesh->PrimitiveIndices.size() * sizeof(MeshletTriangle));
	Writer.Write(Mesh->Submeshes.data(), Mesh->Submeshes.size() * sizeof(Asset::Submesh));
}

AsyncMeshLoader::TResourcePtr AsyncMeshLoader::ImportMesh(
	const std::filesystem::path& BinaryPath,
	const Asset::MeshMetadata&	 Metadata)
{
	FileStream	 Stream(BinaryPath, FileMode::Open, FileAccess::Read);
	BinaryReader Reader(Stream);

	auto								  Header = Reader.Read<MeshHeader>();
	std::vector<Vertex>					  Vertices(Header.NumVertices);
	std::vector<uint32_t>				  Indices(Header.NumIndices);
	std::vector<DirectX::Meshlet>		  Meshlets(Header.NumMeshlets);
	std::vector<uint8_t>				  UniqueVertexIndices(Header.NumUniqueVertexIndices);
	std::vector<DirectX::MeshletTriangle> PrimitiveIndices(Header.NumPrimitiveIndices);
	std::vector<Asset::Submesh>			  Submeshes(Header.NumSubmeshes);

	Reader.Read(Vertices.data(), Vertices.size() * sizeof(Vertex));
	Reader.Read(Indices.data(), Indices.size() * sizeof(std::uint32_t));
	Reader.Read(Meshlets.data(), Meshlets.size() * sizeof(Meshlet));
	Reader.Read(UniqueVertexIndices.data(), UniqueVertexIndices.size() * sizeof(uint8_t));
	Reader.Read(PrimitiveIndices.data(), PrimitiveIndices.size() * sizeof(MeshletTriangle));
	Reader.Read(Submeshes.data(), Submeshes.size() * sizeof(Asset::Submesh));

	auto Mesh	   = std::make_shared<Asset::Mesh>();
	Mesh->Metadata = Metadata;
	Mesh->Name	   = Metadata.Path.filename().string();

	Mesh->Vertices			  = std::move(Vertices);
	Mesh->Indices			  = std::move(Indices);
	Mesh->Meshlets			  = std::move(Meshlets);
	Mesh->UniqueVertexIndices = std::move(UniqueVertexIndices);
	Mesh->PrimitiveIndices	  = std::move(PrimitiveIndices);
	Mesh->Submeshes			  = std::move(Submeshes);

	return Mesh;
}
