#pragma once
#include <basetsd.h>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include <Core/RWLock.h>

template<typename T>
class AssetCache;

template<typename T>
class AssetHandle
{
	friend class AssetCache<T>;

	AssetHandle(std::shared_ptr<T> Resource)
		: Resource(std::move(Resource))
	{
	}

public:
	AssetHandle() noexcept = default;

	auto operator<=>(const AssetHandle&) const = default;

	explicit operator bool() const { return static_cast<bool>(Resource); }

	[[nodiscard]] const T& Get() const { return *Resource; }

	[[nodiscard]] T& Get() { return *Resource; }

	[[nodiscard]] const T& operator*() const { return Get(); }

	[[nodiscard]] T& operator*() { return Get(); }

	T* operator->() const { return Resource.get(); }

	T* operator->() { return Resource.get(); }

private:
	std::shared_ptr<T> Resource;
};

template<typename T>
class AssetCache
{
public:
	using Handle = AssetHandle<T>;

	auto size() const { return Cache.size(); }

	void DestroyAll()
	{
		ScopedWriteLock _(RWLock);

		Cache.clear();
	}

	template<typename... TArgs>
	Handle Create(UINT64 Key, TArgs&&... Args)
	{
		ScopedWriteLock _(RWLock);

		Handle Handle = std::make_shared<T>(std::forward<TArgs>(Args)...);

		if (Cache.find(Key) == Cache.end())
		{
			Cache[Key] = Handle;
		}

		return Handle;
	}

	void Emplace(UINT64 Key, std::shared_ptr<T> Resource)
	{
		ScopedWriteLock _(RWLock);

		if (Cache.find(Key) == Cache.end())
		{
			Cache[Key] = Resource;
		}
	}

	void Discard(UINT64 Key)
	{
		ScopedWriteLock _(RWLock);

		if (auto iter = Cache.find(Key); iter != Cache.end())
		{
			Cache.erase(iter);
		}
	}

	Handle Load(UINT64 Key) const
	{
		ScopedReadLock _(RWLock);

		Handle Handle = {};
		if (auto iter = Cache.find(Key); iter != Cache.end())
		{
			Handle = iter->second;
		}

		return Handle;
	}

	bool Exist(UINT64 Key) const
	{
		ScopedReadLock _(RWLock);

		return Cache.find(Key) != Cache.end();
	}

	template<typename Functor>
	void Each(Functor F) const
	{
		auto begin = Cache.begin();
		auto end   = Cache.end();
		while (begin != end)
		{
			auto current = begin++;

			if constexpr (std::is_invocable_v<Functor, UINT64>)
			{
				F(current->first);
			}
			else if constexpr (std::is_invocable_v<Functor, Handle>)
			{
				F(Handle(current->second));
			}
			else
			{
				F(current->first, Handle(current->second));
			}
		}
	}

private:
	std::unordered_map<UINT64, std::shared_ptr<T>> Cache;
	mutable RWLock								   RWLock;

	friend class AssetWindow;
	friend class SceneParser;
};
