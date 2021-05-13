#pragma once
#include <vector>
#include <cassert>
#include "Synchronization/CriticalSection.h"

template<typename T, size_t Size>
class pool
{
public:
	template<typename T>
	struct SElement
	{
		T Value;
		size_t Next;
	};

	template<>
	struct SElement<void>
	{
		size_t Next;
	};

	using Element = SElement<T>;

	pool()
	{
		m_Elements.resize(Size);
		clear();
	}

	auto& operator[](size_t Index)
	{
		return m_Elements[Index];
	}

	const auto& operator[](size_t Index) const
	{
		return m_Elements[Index];
	}

	void clear()
	{
		m_FreeStart = 0;
		m_NumActiveElements = 0;
		for (size_t i = 0; i < Size; ++i)
		{
			m_Elements[i].Next = i + 1;
		}
	}

	// Removes the first element from the free list and returns its index
	// throws if no free elements remain
	size_t allocate()
	{
		assert(m_NumActiveElements < Size && "Consider increasing the size of the pool");
		m_NumActiveElements++;
		size_t index = m_FreeStart;
		m_FreeStart = m_Elements[index].Next;
		return index;
	}

	void free(size_t Index)
	{
		m_NumActiveElements--;
		m_Elements[Index].Next = m_FreeStart;
		m_FreeStart = Index;
	}

private:
	std::vector<Element> m_Elements;
	size_t m_FreeStart;
	size_t m_NumActiveElements;
};

template<typename T, size_t Size>
class concurrent_pool
{
public:
	void clear()
	{
		std::scoped_lock _(m_CriticalSection);
		m_Pool.clear();
	}

	auto& operator[](size_t Index)
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Pool[Index];
	}

	const auto& operator[](size_t Index) const
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Pool[Index];
	}

	// Removes the first element from the free list and returns its index
	// throws if no free elements remain
	size_t allocate()
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Pool.allocate();
	}

	void free(size_t Index)
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Pool.free(Index);
	}

private:
	pool<T, Size> m_Pool;
	CriticalSection m_CriticalSection;
};
