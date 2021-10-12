#pragma once
#include "Image.h"
#include "Mesh.h"

/*
 * All inherited loader must implement a method called AsyncLoad,
 * it takes in the TMetadata and returns a TResourcePtr
 */
template<typename T, typename TMetadata, typename TDerived>
class AsyncLoader
{
public:
	using TResourcePtr = std::shared_ptr<T>;

	// The delegate is called when AsyncLoad succeeds
	using TDelegate = std::function<void(TResourcePtr)>;

	AsyncLoader()
	{
		Thread = std::jthread(
			[&]()
			{
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

						auto Resource = static_cast<TDerived*>(this)->AsyncLoad(Metadata);
						if (Resource)
						{
							Delegate(Resource);
						}
					}
				}
			});
	}

	~AsyncLoader()
	{
		Quit = true;
		ConditionVariable.notify_one();
	}

	void SetDelegate(TDelegate Delegate) { this->Delegate = Delegate; }

	void RequestAsyncLoad(UINT NumMetadata, TMetadata* pMetadata)
	{
		assert(Delegate != nullptr);
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
	TDelegate				Delegate = nullptr;

	std::jthread	  Thread;
	std::atomic<bool> Quit = false;
};

class AsyncImageLoader : public AsyncLoader<Asset::Image, Asset::ImageMetadata, AsyncImageLoader>
{
public:
	TResourcePtr AsyncLoad(const Asset::ImageMetadata& Metadata);
};

class AsyncMeshLoader : public AsyncLoader<Asset::Mesh, Asset::MeshMetadata, AsyncMeshLoader>
{
public:
	TResourcePtr AsyncLoad(const Asset::MeshMetadata& Metadata);

	void		 ExportMesh(const std::filesystem::path& BinaryPath, TResourcePtr Mesh);
	TResourcePtr ImportMesh(const std::filesystem::path& BinaryPath, const Asset::MeshMetadata& Metadata);

	struct MeshHeader
	{
		size_t NumVertices;
		size_t NumIndices;
		size_t NumMeshlets;
		size_t NumUniqueVertexIndices;
		size_t NumPrimitiveIndices;
		size_t NumSubmeshes;
	};
};
