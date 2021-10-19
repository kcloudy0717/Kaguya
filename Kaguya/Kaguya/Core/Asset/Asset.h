#pragma once

enum class AssetType : UINT64
{
	Unknown,
	Mesh,
	Texture
};

enum class AssetState : UINT64
{
	Dirty,
	Ready
};

template<AssetType Enum>
struct AssetTypeTraits
{
	using Type = void;
};

struct AssetHandle
{
	AssetHandle() { Invalidate(); }

	auto operator<=>(const AssetHandle&) const = default;

	[[nodiscard]] bool IsValid() const noexcept { return Type != AssetType::Unknown && Id != UINT_MAX; }

	void Invalidate()
	{
		Type	= AssetType::Unknown;
		State	= AssetState::Dirty;
		Version = 0;
		Id		= UINT_MAX;
	}

	AssetType  Type	   : 16;
	AssetState State   : 2;
	UINT64	   Version : 14;
	UINT64	   Id	   : 32;
};

static_assert(sizeof(AssetHandle) == sizeof(UINT64));

class Asset
{
public:
	AssetHandle Handle;
};

struct unique_asset_deleter
{
	template<typename T>
	void operator()(T* Ptr) const
	{
		Ptr->~T();
	}
};

template<typename T, class Deleter = unique_asset_deleter>
class AssetPtr : protected std::unique_ptr<T, Deleter>
{
public:
	static_assert(std::is_empty_v<Deleter>, "AssetPtr doesn't support stateful deleter.");
	using parent  = std::unique_ptr<T, Deleter>;
	using pointer = typename parent::pointer;

	AssetPtr()
		: parent(nullptr)
	{
	}

	explicit AssetPtr(T* p)
		: parent(p)
	{
	}

	template<typename UDeleter>
	AssetPtr(AssetPtr<T, UDeleter>&& AssetPtr)
		: parent(AssetPtr.release())
	{
	}

	template<typename UDeleter>
	AssetPtr& operator=(AssetPtr<T, UDeleter>&& AssetPtr)
	{
		parent::reset(AssetPtr.release());
		return *this;
	}

	AssetPtr& operator=(AssetPtr const&) = delete;
	AssetPtr(AssetPtr const&)			 = delete;

	AssetPtr& operator=(pointer p)
	{
		reset(p);
		return *this;
	}

	AssetPtr& operator=(std::nullptr_t p)
	{
		reset(p);
		return *this;
	}

	void reset(pointer p = pointer()) { parent::reset(p); }

	void reset(std::nullptr_t p) { parent::reset(p); }

	using parent::get;
	using parent::release;
	using parent::operator->;
	using parent::operator*;
	using parent::operator bool;
};
