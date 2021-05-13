#pragma once
#include "Synchronization/CriticalSection.h"
#include "Synchronization/ConditionVariable.h"
#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() noexcept = default;

	ThreadSafeQueue(ThreadSafeQueue&&) noexcept = default;
	ThreadSafeQueue& operator=(ThreadSafeQueue&&) noexcept = default;

	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	void push(const T& Item)
	{
		std::scoped_lock _(m_CriticalSection);
		m_Queue.push(Item);
		::WakeConditionVariable(&m_ConditionVariable);
	}

	void push(T&& Item)
	{
		std::scoped_lock _(m_CriticalSection);
		m_Queue.push(std::move(Item));
		::WakeConditionVariable(&m_ConditionVariable);
	}

	bool pop(T& Item, int Milliseconds)
	{
		std::scoped_lock _(m_CriticalSection);

		while (m_Queue.empty())
		{
			BOOL result = ::SleepConditionVariableCS(&m_ConditionVariable, m_CriticalSection, Milliseconds);
			if (result == ERROR_TIMEOUT || m_Queue.empty())
				return false;
		}

		Item = std::move(m_Queue.front());
		m_Queue.pop();

		return true;
	}

	size_t size() const
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Queue.size();
	}

	bool empty() const
	{
		std::scoped_lock _(m_CriticalSection);
		return m_Queue.empty();
	}

private:
	std::queue<T> m_Queue;
	mutable CriticalSection m_CriticalSection;
	ConditionVariable m_ConditionVariable;
};
