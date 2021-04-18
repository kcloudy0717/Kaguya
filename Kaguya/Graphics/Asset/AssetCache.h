#pragma once
#include <basetsd.h>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include <Core/Synchronization/RWLock.h>
#include <Core/Utility.h>

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
	AssetHandle() = default;

	auto operator<=>(const AssetHandle&) const = default;

	explicit operator bool() const
	{
		return static_cast<bool>(Resource);
	}

	[[nodiscard]] const T& Get() const
	{
		return *Resource;
	}

	[[nodiscard]] T& Get()
	{
		return *Resource;
	}

	[[nodiscard]] const T& operator*() const
	{
		return Get();
	}

	[[nodiscard]] T& operator*()
	{
		return Get();
	}

	T* operator->() const
	{
		return Resource.get();
	}

	T* operator->()
	{
		return Resource.get();
	}
private:
	std::shared_ptr<T> Resource;
};

template<typename T>
class AssetCache
{
public:
	using Handle = AssetHandle<T>;

	AssetCache() = default;

	AssetCache(AssetCache&&) = default;

	AssetCache& operator=(AssetCache&&) = default;

	auto size() const
	{
		return m_Cache.size();
	}

	void DestroyAll()
	{
		ScopedWriteLock _(m_RWLock);

		m_Cache.clear();
	}

	template<typename... Args>
	void Create(UINT64 Key, Args&&... args)
	{
		ScopedWriteLock _(m_RWLock);

		if (m_Cache.find(Key) == m_Cache.end())
		{
			m_Cache[Key] = std::make_shared<T>(std::forward<Args>(args)...);
		}
	}

	void Discard(UINT64 Key)
	{
		ScopedWriteLock _(m_RWLock);

		if (auto it = m_Cache.find(Key);
			it != m_Cache.end())
		{
			m_Cache.erase(it);
		}
	}

	template<typename... Args>
	Handle Load(UINT64 Key, Args&&... args) const
	{
		ScopedReadLock _(m_RWLock);

		Handle Handle = {};
		if (auto it = m_Cache.find(Key);
			it != m_Cache.end())
		{
			Handle = it->second;
		}

		return Handle;
	}

	bool Exist(UINT64 Key) const
	{
		ScopedReadLock _(m_RWLock);

		return m_Cache.find(Key) != m_Cache.end();
	}

	template<typename Functor>
	void Each(Functor F) const
	{
		auto begin = m_Cache.begin();
		auto end = m_Cache.end();
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

	// Acquires RWLock
	template<typename Functor>
	void Each_ThreadSafe(Functor F) const
	{
		ScopedWriteLock _(m_RWLock);
		auto begin = m_Cache.begin();
		auto end = m_Cache.end();
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
	std::unordered_map<UINT64, std::shared_ptr<T>> m_Cache;
	mutable RWLock m_RWLock;

	friend class AssetWindow;
	friend class SceneParser;
};