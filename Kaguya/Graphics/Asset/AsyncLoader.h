#pragma once
#include <wil/resource.h>

#include "Image.h"
#include "Mesh.h"

#include "AssetCache.h"

/*
* All inherited loader must implement a method called AsyncLoad,
* it takes in the TMetadata and returns a TResourcePtr
*/
template<typename T, typename Metadata, typename Loader>
class AsyncLoader
{
public:
	using TResource = T;
	using TResourcePtr = std::shared_ptr<TResource>;
	using TMetadata = Metadata;
	using TDelegate = std::function<void(TResourcePtr)>;

	AsyncLoader()
	{
		m_Thread.reset(::CreateThread(nullptr, 0, &AsyncThreadProc, this, 0, nullptr));
	}

	~AsyncLoader()
	{
		m_Shutdown = true;
		m_ConditionVariable.WakeAll();

		::WaitForSingleObject(m_Thread.get(), INFINITE);
	}

	void SetCallback(TDelegate Delegate)
	{
		this->m_Delegate = Delegate;
	}

	void RequestAsyncLoad(UINT NumMetadata, Metadata* pMetadata)
	{
		std::scoped_lock _(m_CriticalSection);
		for (UINT i = 0; i < NumMetadata; i++)
		{
			m_MetadataQueue.push(pMetadata[i]);
		}
		m_ConditionVariable.Wake();
	}
private:
	static DWORD WINAPI AsyncThreadProc(_In_ PVOID pParameter)
	{
		auto pAsyncLoader = static_cast<AsyncLoader<T, Metadata, Loader>*>(pParameter);

		while (true)
		{
			// Sleep until the condition variable becomes notified
			std::scoped_lock _(pAsyncLoader->m_CriticalSection);
			pAsyncLoader->m_ConditionVariable.Wait(pAsyncLoader->m_CriticalSection, INFINITE);

			if (pAsyncLoader->m_Shutdown)
			{
				break;
			}

			// Start loading stuff async :D
			TMetadata metadata = {};
			while (pAsyncLoader->m_MetadataQueue.pop(metadata, 0))
			{
				auto pResource = static_cast<Loader*>(pAsyncLoader)->AsyncLoad(metadata);
				if (pResource && pAsyncLoader->m_Delegate)
				{
					pAsyncLoader->m_Delegate(pResource);
				}
			}
		}

		return EXIT_SUCCESS;
	}
private:
	CriticalSection m_CriticalSection;
	ConditionVariable m_ConditionVariable;
	ThreadSafeQueue<Metadata> m_MetadataQueue;
	TDelegate m_Delegate = nullptr;

	wil::unique_handle m_Thread;
	std::atomic<bool> m_Shutdown = false;
};

class AsyncImageLoader : public AsyncLoader<Asset::Image, Asset::ImageMetadata, AsyncImageLoader>
{
private:
	TResourcePtr AsyncLoad(const TMetadata& Metadata);

	friend class AsyncLoader<Asset::Image, Asset::ImageMetadata, AsyncImageLoader>;
};

class AsyncMeshLoader : public AsyncLoader<Asset::Mesh, Asset::MeshMetadata, AsyncMeshLoader>
{
private:
	TResourcePtr AsyncLoad(const TMetadata& Metadata);

	friend class AsyncLoader<Asset::Mesh, Asset::MeshMetadata, AsyncMeshLoader>;
};