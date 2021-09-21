#pragma once
#include <memory>
#include <type_traits>

template<typename T>
class TObjectPool
{
public:
	using ValueType = T;
	using Pointer	= T*;

	explicit TObjectPool(std::size_t Size)
		: Size(Size)
		, ActiveElements(0)
	{
		Pool = std::make_unique<Element[]>(Size);

		for (std::size_t i = 1; i < Size; ++i)
		{
			Pool[i - 1].pNext = &Pool[i];
		}
		FreeStart = &Pool[0];
	}
	~TObjectPool() = default;

	TObjectPool(TObjectPool&& TPool) noexcept
		: Pool(std::exchange(TPool.Pool, {}))
		, FreeStart(std::exchange(TPool.FreeStart, {}))
		, Size(std::exchange(TPool.Size, {}))
		, ActiveElements(std::exchange(TPool.ActiveElements, {}))
	{
	}
	TObjectPool& operator=(TObjectPool&& TPool) noexcept
	{
		if (this != &TPool)
		{
			Pool		   = std::exchange(TPool.Pool, {});
			FreeStart	   = std::exchange(TPool.FreeStart, {});
			Size		   = std::exchange(TPool.Size, {});
			ActiveElements = std::exchange(TPool.ActiveElements, {});
		}
		return *this;
	}

	TObjectPool(const TObjectPool&) = delete;
	TObjectPool& operator=(const TObjectPool&) = delete;

	[[nodiscard]] Pointer Allocate()
	{
		if (!FreeStart)
		{
			throw std::bad_alloc();
		}

		const auto AvailableElement = FreeStart;
		FreeStart					= AvailableElement->pNext;

		ActiveElements++;
		return reinterpret_cast<Pointer>(&AvailableElement->Storage);
	}

	void Deallocate(Pointer Pointer) noexcept
	{
		const auto pElement = reinterpret_cast<Element*>(Pointer);
		pElement->pNext		= FreeStart;
		FreeStart			= pElement;
		--ActiveElements;
	}

	template<typename... TArgs>
	[[nodiscard]] Pointer Construct(TArgs&&... Args)
	{
		return new (Allocate()) ValueType(std::forward<TArgs>(Args)...);
	}

	void Destruct(Pointer p) noexcept
	{
		if (p)
		{
			Deallocate(p);
		}
	}

private:
	union Element
	{
		std::aligned_storage_t<sizeof(ValueType), alignof(ValueType)> Storage;
		Element*													  pNext;
	};

	std::unique_ptr<Element[]> Pool;
	Element*				   FreeStart;
	std::size_t				   Size;
	std::size_t				   ActiveElements;
};
