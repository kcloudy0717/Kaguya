#include "ThreadPool.h"
#include <vector>
#include <atomic>
#include <thread>
#include <future>
#include <deque>
#include <queue>

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue()
		: Mutex(std::make_unique<std::mutex>())
	{
	}

	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	ThreadSafeQueue(ThreadSafeQueue&&) noexcept = default;
	ThreadSafeQueue& operator=(ThreadSafeQueue&&) noexcept = default;

	void Push(T Item)
	{
		std::scoped_lock Lock(*Mutex);
		Queue.push(std::move(Item));
	}

	bool TryPop(T& Item)
	{
		std::scoped_lock Lock(*Mutex);
		if (Queue.empty())
		{
			return false;
		}
		Item = std::move(Queue.front());
		Queue.pop();
		return true;
	}

private:
	std::queue<T>						Queue;
	mutable std::unique_ptr<std::mutex> Mutex;
};

template<typename T>
class WorkStealingQueue
{
public:
	WorkStealingQueue()
		: Mutex(std::make_unique<std::mutex>())
	{
	}

	WorkStealingQueue(const WorkStealingQueue&) = delete;
	WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

	WorkStealingQueue(WorkStealingQueue&&) noexcept = default;
	WorkStealingQueue& operator=(WorkStealingQueue&&) noexcept = default;

	void Push(T Item)
	{
		std::scoped_lock Lock(*Mutex);
		Queue.push_front(std::move(Item));
	}

	bool TryPop(T& Item)
	{
		std::scoped_lock Lock(*Mutex);
		if (Queue.empty())
		{
			return false;
		}
		Item = std::move(Queue.front());
		Queue.pop_front();
		return true;
	}

	bool TrySteal(T& Item)
	{
		std::scoped_lock Lock(*Mutex);
		if (Queue.empty())
		{
			return false;
		}
		Item = std::move(Queue.back());
		Queue.pop_back();
		return true;
	}

private:
	std::deque<T>						Queue;
	mutable std::unique_ptr<std::mutex> Mutex;
};

class stdThreadPool
{
public:
	using ThreadpoolWork = std::function<void(void*)>;

	struct WorkEntry
	{
		ThreadpoolWork Work;
		void*		   Context = nullptr;
	};

	stdThreadPool()
		: IsDone(false)
	{
		unsigned thread_count = std::thread::hardware_concurrency();

		try
		{
			Queues.resize(thread_count);
			Threads.reserve(thread_count);
			for (unsigned i = 0; i < thread_count; ++i)
			{
				Threads.emplace_back(std::jthread(&stdThreadPool::WorkerThread, this, i));
			}
		}
		catch (...)
		{
			IsDone = true;
			throw;
		}
	}

	~stdThreadPool()
	{
		IsDone = true;
	}

	void QueueThreadpoolWork(ThreadpoolWork&& Callback, void* Context)
	{
		WorkEntry Entry = {
			.Work	 = std::move(Callback),
			.Context = Context
		};
		if (ThreadLocalWorkQueue)
		{
			ThreadLocalWorkQueue->Push(std::move(Entry));
		}
		else
		{
			WorkPoolQueue.Push(std::move(Entry));
		}
	}

private:
	std::atomic_bool						  IsDone;
	ThreadSafeQueue<WorkEntry>				  WorkPoolQueue;
	std::vector<WorkStealingQueue<WorkEntry>> Queues;
	std::vector<std::jthread>				  Threads;

	static thread_local WorkStealingQueue<WorkEntry>* ThreadLocalWorkQueue;
	static thread_local unsigned					  ThreadLocalIndex;

	void WorkerThread(unsigned Index)
	{
		ThreadLocalIndex	 = Index;
		ThreadLocalWorkQueue = &Queues[ThreadLocalIndex];
		while (!IsDone)
		{
			WorkEntry Work;
			if (PopTaskFromLocalQueue(Work) ||
				PopTaskFromPoolQueue(Work) ||
				PopTaskFromOtherThreadQueue(Work))
			{
				Work.Work(Work.Context);
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}

	bool PopTaskFromLocalQueue(WorkEntry& Work)
	{
		return ThreadLocalWorkQueue && ThreadLocalWorkQueue->TryPop(Work);
	}

	bool PopTaskFromPoolQueue(WorkEntry& Work)
	{
		return WorkPoolQueue.TryPop(Work);
	}

	bool PopTaskFromOtherThreadQueue(WorkEntry& Work)
	{
		for (unsigned i = 0; i < Queues.size(); ++i)
		{
			unsigned const index = (ThreadLocalIndex + i + 1) % Queues.size();
			if (Queues[index].TrySteal(Work))
			{
				return true;
			}
		}

		return false;
	}
};
