#pragma once
#include <coroutine>
#include "Async.h"

class AsyncAction
{
public:
	struct promise_type final : promise_base<promise_type>
	{
		void return_void() noexcept {}
	};

	using coroutine_handle = std::coroutine_handle<promise_type>;

	AsyncAction() noexcept = default;
	AsyncAction(coroutine_handle Handle)
		: Handle(Handle)
	{
	}

	~AsyncAction() { InternalDestroy(); }

	AsyncAction(AsyncAction&& AsyncAction) noexcept
		: Handle(std::exchange(AsyncAction.Handle, nullptr))
	{
	}
	AsyncAction& operator=(AsyncAction&& AsyncAction) noexcept
	{
		if (this == &AsyncAction)
		{
			return *this;
		}

		InternalDestroy();
		Handle = std::exchange(AsyncAction.Handle, nullptr);
		return *this;
	}

	AsyncAction(std::nullptr_t) {}
	AsyncAction& operator=(std::nullptr_t)
	{
		InternalDestroy();
		return *this;
	}

	AsyncAction(const AsyncAction&) = delete;
	AsyncAction& operator=(const AsyncAction&) = delete;

	[[nodiscard]] operator bool() const noexcept { return static_cast<bool>(Handle); }

	[[nodiscard]] AsyncStatus GetStatus() const noexcept { return Handle.promise().GetStatus(); }

	void operator()() const { Handle.resume(); }

	void Resume() const { Handle.resume(); }

	[[nodiscard]] bool Done() const noexcept { return Handle.done(); }

	void Wait() const noexcept
	{
		bool Succeeded = Handle.promise().Event.wait();
		assert(Succeeded);
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
