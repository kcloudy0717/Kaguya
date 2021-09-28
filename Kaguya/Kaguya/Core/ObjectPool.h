#pragma once
#include <memory>
#include <type_traits>

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

		for (std::size_t i = 1; i < Size; ++i)
		{
			Pool[i - 1].Next = &Pool[i];
		}
		FreeStart = &Pool[0];
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
	[[nodiscard]] Pointer Construct(TArgs&&... Args)
	{
		return new (Allocate()) ValueType(std::forward<TArgs>(Args)...);
	}

	void Destruct(Pointer Pointer) noexcept
	{
		if (Pointer)
		{
			Pointer->~T();
			Deallocate(Pointer);
		}
	}

private:
	[[nodiscard]] Pointer Allocate()
	{
		if (!FreeStart)
		{
			throw std::bad_alloc();
		}

		const auto AvailableElement = FreeStart;
		FreeStart					= AvailableElement->Next;

		ActiveElements++;
		return reinterpret_cast<Pointer>(&AvailableElement->Storage);
	}

	void Deallocate(Pointer Pointer) noexcept
	{
		const auto pElement = reinterpret_cast<Element*>(Pointer);
		pElement->Next		= FreeStart;
		FreeStart			= pElement;
		--ActiveElements;
	}

private:
	union Element
	{
		std::aligned_storage_t<sizeof(ValueType), alignof(ValueType)> Storage;
		Element*													  Next;
	};

	std::unique_ptr<Element[]> Pool;
	Element*				   FreeStart;
	std::size_t				   Size;
	std::size_t				   ActiveElements;
};
