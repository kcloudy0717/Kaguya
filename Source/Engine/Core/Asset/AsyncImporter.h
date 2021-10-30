#pragma once
#include "Texture.h"
#include "Mesh.h"

template<typename T, typename TImportOptions, typename TDerived>
class AsyncImporter
{
public:
	AsyncImporter(const std::wstring& ThreadName)
	{
		Thread = std::jthread(
			[&, ThreadName]()
			{
				SetThreadDescription(GetCurrentThread(), ThreadName.data());
				while (true)
				{
					std::unique_lock Lock(Mutex);
					ConditionVariable.wait(Lock);
					if (Quit)
					{
						break;
					}

					// Start loading stuff async :D
					while (!ImportRequests.empty())
					{
						auto Options = ImportRequests.front();
						ImportRequests.pop();
						static_cast<TDerived*>(this)->Import(Options);
					}
				}
			});
	}

	~AsyncImporter()
	{
		Quit = true;
		ConditionVariable.notify_one();
	}

	void RequestAsyncLoad(const TImportOptions& Options)
	{
		std::scoped_lock Lock(Mutex);
		ImportRequests.push(Options);
		ConditionVariable.notify_one();
	}

	bool SupportsExtension(const std::filesystem::path& Path)
	{
		std::wstring Extension = Path.extension().wstring();
		return std::ranges::find_if(
				   SupportedExtensions.begin(),
				   SupportedExtensions.end(),
				   [&](const std::wstring& SupportedExtension)
				   {
					   return SupportedExtension == Extension;
				   }) != SupportedExtensions.end();
	}

private:
	std::mutex				   Mutex;
	std::condition_variable	   ConditionVariable;
	std::queue<TImportOptions> ImportRequests;

	std::jthread	  Thread;
	std::atomic<bool> Quit = false;

protected:
	std::set<std::wstring> SupportedExtensions;
};

class AsyncTextureImporter : public AsyncImporter<Texture, TextureImportOptions, AsyncTextureImporter>
{
public:
	AsyncTextureImporter()
		: AsyncImporter(L"Texture Importer")
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

	void Import(const TextureImportOptions& Options);
};

class AsyncMeshImporter : public AsyncImporter<Mesh, MeshImportOptions, AsyncMeshImporter>
{
public:
	AsyncMeshImporter()
		: AsyncImporter(L"Mesh Importer")
	{
		SupportedExtensions.insert(L".fbx");
		SupportedExtensions.insert(L".obj");
	}

	void Import(const MeshImportOptions& Options);

	void			   Export(const std::filesystem::path& BinaryPath, const std::vector<Mesh*>& Meshes);
	std::vector<Mesh*> ImportExisting(const std::filesystem::path& BinaryPath, const MeshImportOptions& Options);

	struct ExportHeader
	{
		size_t NumMeshes;
	};

	struct MeshHeader
	{
		size_t NumVertices;
		size_t NumIndices;
		size_t NumMeshlets;
		size_t NumUniqueVertexIndices;
		size_t NumPrimitiveIndices;
	};
};
