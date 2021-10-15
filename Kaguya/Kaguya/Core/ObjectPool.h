#pragma once
#include <memory>
#include <type_traits>

// If the type is not trivial, destructor must be invoked explicitly
template<typename T>
class ObjectPool
{
public:
	using ValueType = T;
	using Pointer	= T*;

	explicit ObjectPool(std::size_t Size)
		: Size(Size)
		, ActiveElements(0)
	{
		Pool = std::make_unique<Element[]>(Size);
		for (std::size_t i = 0; i < Size; ++i)
		{
			Pool[i].Next = i + 1;
		}
		FreeStart = 0;
	}
	~ObjectPool() = default;

	ObjectPool(ObjectPool&& ObjectPool) noexcept
		: Pool(std::exchange(ObjectPool.Pool, {}))
		, FreeStart(std::exchange(ObjectPool.FreeStart, {}))
		, Size(std::exchange(ObjectPool.Size, {}))
		, ActiveElements(std::exchange(ObjectPool.ActiveElements, {}))
	{
	}
	ObjectPool& operator=(ObjectPool&& ObjectPool) noexcept
	{
		if (this == &ObjectPool)
		{
			return *this;
		}

		Pool		   = std::exchange(ObjectPool.Pool, {});
		FreeStart	   = std::exchange(ObjectPool.FreeStart, {});
		Size		   = std::exchange(ObjectPool.Size, {});
		ActiveElements = std::exchange(ObjectPool.ActiveElements, {});
		return *this;
	}

	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;

	template<typename... TArgs>
	[[nodiscard]] Pointer Construct(std::size_t Index, TArgs&&... Args)
	{
		auto Storage = reinterpret_cast<Pointer>(&Pool[Index].Storage);
		new (Storage) ValueType(std::forward<TArgs>(Args)...);
		return Storage;
	}

	Pointer operator[](std::size_t Index) { return reinterpret_cast<Pointer>(&Pool[Index].Storage); }

	[[nodiscard]] std::size_t Allocate()
	{
		ActiveElements++;
		std::size_t Index = FreeStart;
		FreeStart		  = Pool[Index].Next;
		return Index;
	}

	void Deallocate(std::size_t Index) noexcept
	{
		Pool[Index].Next = FreeStart;
		FreeStart		 = Index;
		--ActiveElements;
	}

private:
	union Element
	{
		std::aligned_storage_t<sizeof(ValueType), alignof(ValueType)> Storage;
		std::size_t													  Next;
	};

	std::unique_ptr<Element[]> Pool;
	std::size_t				   FreeStart;
	std::size_t				   Size;
	std::size_t				   ActiveElements;
};
