#pragma once
#include <wil/resource.h>

#include "Image.h"
#include "Mesh.h"

#include "AssetCache.h"

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
		struct threadwrapper
		{
			static unsigned int WINAPI thunk(LPVOID lpParameter)
			{
				auto pAsyncLoader = static_cast<AsyncLoader<T, TMetadata, TDerived>*>(lpParameter);

				while (true)
				{
					// Sleep until the condition variable becomes notified
					std::unique_lock _(pAsyncLoader->CriticalSection);
					pAsyncLoader->ConditionVariable.wait(_);

					if (pAsyncLoader->Quit)
					{
						break;
					}

					// Start loading stuff async :D
					while (!pAsyncLoader->MetadataQueue.empty())
					{
						auto Metadata = pAsyncLoader->MetadataQueue.front();
						pAsyncLoader->MetadataQueue.pop();

						auto pResource = static_cast<TDerived*>(pAsyncLoader)->AsyncLoad(Metadata);
						if (pResource)
						{
							pAsyncLoader->Delegate(pResource);
						}
					}
				}

				return EXIT_SUCCESS;
			}
		};

		Thread.reset(reinterpret_cast<HANDLE>(
			_beginthreadex(nullptr, 0, threadwrapper::thunk, reinterpret_cast<LPVOID>(this), 0, nullptr)));
	}

	~AsyncLoader()
	{
		Quit = true;
		ConditionVariable.notify_all();

		::WaitForSingleObject(Thread.get(), INFINITE);
	}

	void SetDelegate(TDelegate Delegate) { this->Delegate = Delegate; }

	void RequestAsyncLoad(UINT NumMetadata, TMetadata* pMetadata)
	{
		assert(Delegate != nullptr);
		std::scoped_lock _(CriticalSection);
		for (UINT i = 0; i < NumMetadata; i++)
		{
			MetadataQueue.push(pMetadata[i]);
		}
		ConditionVariable.notify_one();
	}

private:
	std::mutex				CriticalSection;
	std::condition_variable ConditionVariable;
	std::queue<TMetadata>	MetadataQueue;
	TDelegate				Delegate = nullptr;

	wil::unique_handle Thread;
	std::atomic<bool>  Quit = false;
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
};
