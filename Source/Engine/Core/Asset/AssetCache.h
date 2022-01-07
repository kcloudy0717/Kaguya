#pragma once
#include <Core/ObjectPool.h>
#include "Asset.h"

template<AssetType Type, typename T>
class AssetCache
{
public:
	static_assert(std::is_base_of_v<Asset, T>, "Asset is not based of T");

	// static constexpr size_t NumAssets = 4096;
	static constexpr size_t NumAssets = 8192;

	AssetCache()
		: Cache(NumAssets)
		, CachedHandles(NumAssets)
		, Assets(NumAssets)
	{
	}

	void DestroyAll()
	{
		// Destroy in opposite order
		RwLockWriteGuard Guard(Mutex);
		Assets.clear();
		Assets.resize(NumAssets);
		CachedHandles.clear();
		CachedHandles.resize(NumAssets);
		Cache = ObjectPool<T>(NumAssets);
	}

	auto begin() noexcept { return CachedHandles.begin(); }
	auto end() noexcept { return CachedHandles.end(); }

	size_t size() const noexcept { return CachedHandles.size(); }

	static bool ValidateHandle(AssetHandle Handle) noexcept { return Handle.Type == Type && Handle.Id < NumAssets; }

	template<typename... TArgs>
	AssetHandle Create(TArgs&&... Args)
	{
		RwLockWriteGuard Guard(Mutex);

		std::size_t Index = Cache.Allocate();

		AssetHandle Handle;
		Handle.Type	   = Type;
		Handle.State   = false;
		Handle.Version = 0;
		Handle.Id	   = Index;

		T* Asset	  = Cache.Construct(Index, std::forward<TArgs>(Args)...);
		Asset->Handle = Handle;

		CachedHandles[Index] = Handle;
		Assets[Index]		 = Asset;
		return Handle;
	}

	T* GetAsset(AssetHandle Handle)
	{
		return Assets[Handle.Id].get();
	}

	T* GetValidAsset(AssetHandle& Handle)
	{
		if (Handle.IsValid() && ValidateHandle(Handle))
		{
			RwLockReadGuard Guard(Mutex);

			bool State = CachedHandles[Handle.Id].State;
			if (State)
			{
				Handle.State = State;
				return Assets[Handle.Id].get();
			}
		}

		return nullptr;
	}

	void Destroy(AssetHandle Handle)
	{
		if (Handle.IsValid() && ValidateHandle(Handle))
		{
			RwLockWriteGuard Guard(Mutex);

			Cache.Deallocate(Handle.Id);
			CachedHandles[Handle.Id].Invalidate();
			Assets[Handle.Id].reset();
		}
	}

	void UpdateHandleState(AssetHandle Handle)
	{
		if (Handle.IsValid() && ValidateHandle(Handle))
		{
			RwLockWriteGuard Guard(Mutex);

			CachedHandles[Handle.Id].State = Handle.State;
		}
	}

	template<typename Functor>
	void EnumerateAsset(Functor F)
	{
		RwLockReadGuard Guard(Mutex);

		auto begin = CachedHandles.begin();
		auto end   = CachedHandles.end();
		while (begin != end)
		{
			auto current = begin++;

			if constexpr (std::is_invocable_v<Functor, AssetHandle, T*>)
			{
				if (current->IsValid())
				{
					F(*current, Cache[current->Id]);
				}
			}
		}
	}

private:
	ObjectPool<T> Cache;
	// CachedHandles are use to update any external handle's states
	std::vector<AssetHandle> CachedHandles;
	std::vector<AssetPtr<T>> Assets;
	mutable RwLock			 Mutex;

	friend class AssetWindow;
	friend class SceneParser;
};
