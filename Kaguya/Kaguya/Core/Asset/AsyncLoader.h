#pragma once
#include "Texture.h"
#include "Mesh.h"

template<typename T, typename TMetadata, typename TDerived>
class AsyncLoader
{
public:
	AsyncLoader(const std::wstring& ThreadName)
	{
		Thread = std::jthread(
			[&, ThreadName]()
			{
				SetThreadDescription(GetCurrentThread(), ThreadName.data());
				while (true)
				{
					std::unique_lock UniqueLock(Mutex);
					ConditionVariable.wait(UniqueLock);
					if (Quit)
					{
						break;
					}

					// Start loading stuff async :D
					while (!MetadataQueue.empty())
					{
						auto Metadata = MetadataQueue.front();
						MetadataQueue.pop();

						static_cast<TDerived*>(this)->AsyncLoad(Metadata);
					}
				}
			});
	}

	~AsyncLoader()
	{
		Quit = true;
		ConditionVariable.notify_one();
	}

	void RequestAsyncLoad(UINT NumMetadata, TMetadata* pMetadata)
	{
		std::scoped_lock _(Mutex);
		for (UINT i = 0; i < NumMetadata; i++)
		{
			MetadataQueue.push(pMetadata[i]);
		}
		ConditionVariable.notify_one();
	}

private:
	std::mutex				Mutex;
	std::condition_variable ConditionVariable;
	std::queue<TMetadata>	MetadataQueue;

	std::jthread	  Thread;
	std::atomic<bool> Quit = false;
};

class AsyncImageLoader : public AsyncLoader<Texture, TextureMetadata, AsyncImageLoader>
{
public:
	AsyncImageLoader()
		: AsyncLoader(L"Async Image Loading Thread")
	{
	}

	void AsyncLoad(const TextureMetadata& Metadata);
};

class AsyncMeshLoader : public AsyncLoader<Mesh, MeshMetadata, AsyncMeshLoader>
{
public:
	AsyncMeshLoader()
		: AsyncLoader(L"Async Mesh Loading Thread")
	{
	}

	void AsyncLoad(const MeshMetadata& Metadata);

	void			   Export(const std::filesystem::path& BinaryPath, const std::vector<Mesh*>& Meshes);
	std::vector<Mesh*> Import(const std::filesystem::path& BinaryPath, const MeshMetadata& Metadata);

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
