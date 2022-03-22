#pragma once
#include <coroutine>
#include "Async.h"

template<typename T>
class AsyncTask
{
public:
	struct promise_type final : promise_base<promise_type, T>
	{
		void return_value(const T& Value) noexcept { this->Value = Value; }

		void return_value(T&& Value) noexcept { this->Value = std::move(Value); }

		T get_return_value() noexcept { return std::move(Value); }

		T copy_return_value() noexcept { return Value; }

		T Value = {};
	};

	using coroutine_handle = std::coroutine_handle<promise_type>;

	AsyncTask() noexcept = default;
	AsyncTask(coroutine_handle Handle)
		: Handle(Handle)
	{
	}

	~AsyncTask() { InternalDestroy(); }

	AsyncTask(AsyncTask&& AsyncTask) noexcept
		: Handle(std::exchange(AsyncTask.Handle, nullptr))
	{
	}
	AsyncTask& operator=(AsyncTask&& AsyncTask) noexcept
	{
		if (this == &AsyncTask)
		{
			return *this;
		}

		InternalDestroy();
		Handle = std::exchange(AsyncTask.Handle, nullptr);
		return *this;
	}

	AsyncTask(std::nullptr_t) {}
	AsyncTask& operator=(std::nullptr_t)
	{
		InternalDestroy();
		return *this;
	}

	AsyncTask(const AsyncTask&) = delete;
	AsyncTask& operator=(const AsyncTask&) = delete;

	[[nodiscard]] operator bool() const noexcept { return static_cast<bool>(Handle); }

	[[nodiscard]] AsyncStatus GetStatus() const noexcept { return Handle.promise().GetStatus(); }

	void operator()() const { Handle.resume(); }

	void Resume() const { Handle.resume(); }

	[[nodiscard]] bool Done() const noexcept { return Handle.done(); }

	// Wait must be called before invoking Get()
	void Wait() const noexcept
	{
		bool Succeeded = Handle.promise().Event.wait();
		assert(Succeeded);
	}

	[[nodiscard]] auto Get() noexcept
	{
		return Handle.promise().Get();
	}

private:
	void InternalDestroy()
	{
		if (Handle)
		{
			Handle.destroy();
			Handle = nullptr;
		}
	}

private:
	coroutine_handle Handle;
};
