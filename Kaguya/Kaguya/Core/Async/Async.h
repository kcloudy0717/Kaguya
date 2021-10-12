#pragma once
#include <atomic>
#include <cassert>
#include <coroutine>
#include <stdexcept>

// Learning coroutines
// These are highly unstable

/*
 * A coroutine is a generalization of a subroutine
 *
 * A subroutine...
 *	- can be invoked by its caller
 *	- can return control back to its caller
 *
 * A coroutine has these properties, but also...
 *	- can suspend execution and return control to its caller
 *	- can resume execution after being suspended
 */

/*
 * When a coroutine is executing, it uses co_await to suspend itself and return control
 * to its caller
 *
 *	auto result = co_await expression;
 *	above code gets translated to something like this
 *
 *	auto&& __a = expression;
 *	if (!__a.await_ready())
 *	{
 *		__a.await_suspend(coroutine-handle);
 *		// ...suspend/resume point...
 *	}
 *
 *	auto result = __a.await_resume();
 *
 * A type that supports the co_await operator is called an Awaitable type
 * The expression must be a type which implements the awaitable_concept
 *
 * To be more specific where required the term Normally Awaitable to describe a type that supports the
 * co_await operator in a coroutine context whose promise type does not have an await_transform member. And I like to
 * use the term Contextually Awaitable to describe a type that only supports the co_await operator in the context of
 * certain types of coroutines due to the presence of an await_transform method in the coroutine's promise type. There
 * are 3 types that co_await operator supports
 * 1. implementing awaitable concept
 * 2. overload operator co_await for that takes in the specific type as an argument and returns a awaitable concept
 * 3. implement await_transform function that is implemented in the promise_type which allows you to customize the
 * behavior for a particular promise_type
 *
 * struct awaitable_concept
 * {
 * 	bool await_ready()
 * 	{
 *
 * 	}
 *
 * 	void await_suspend(std::coroutine_handle<>)
 * 	{
 *
 * 	}
 *
 * 	auto await_resume()
 * 	{
 *
 * 	}
 * };
 */

/*
 * Coroutine lifetime
 * A coroutine comes into existence when it is called
 *	- This is when the compiler creates the coroutine context
 *
 * A coroutine is destroyed when
 *	- the final_suspend is resumed, or
 *	- coroutine_handle<>::destroy() is called.
 *
 *	... whichever happens first.
 *
 * When a coroutine is destroyed, it cleans up local variables
 *	...but only those that were initialized prior to the last suspension point
 */

enum class AsyncStatus
{
	Started,
	Completed,
	Canceled,
	Error,
};

class cencelled_operation : public std::logic_error
{
public:
	cencelled_operation()
		: std::logic_error("cencelled operation")
	{
	}
};

class cancelled_promise : public std::logic_error
{
public:
	cancelled_promise()
		: std::logic_error("cancelled promise")
	{
	}
};

class invalid_method_call : public std::logic_error
{
public:
	invalid_method_call()
		: std::logic_error("invalid method call")
	{
	}
};

// Provides boiler plate code for promise types
template<typename TDerived, typename TResult = void>
struct promise_base
{
	promise_base() { Event.create(); }

	auto get_return_object() noexcept
	{
		return std::coroutine_handle<TDerived>::from_promise(*static_cast<TDerived*>(this));
	}

	auto initial_suspend() noexcept { return std::suspend_never{}; }

	auto final_suspend() noexcept
	{
		struct final_suspend_awaiter
		{
			[[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

			constexpr void await_resume() const noexcept {}

			[[nodiscard]] constexpr bool await_suspend(std::coroutine_handle<>) const noexcept
			{
				AsyncStatus status = Promise->Status.load();
				if (status == AsyncStatus::Started)
				{
					status = AsyncStatus::Completed;
					Promise->Status.store(status);
				}

				Promise->Event.SetEvent();
				return Promise->GetStatus() == AsyncStatus::Completed;
			}

			promise_base* Promise;
		};

		return final_suspend_awaiter{ this };
	}

	void unhandled_exception() noexcept
	{
		Exception = std::current_exception();

		try
		{
			std::rethrow_exception(Exception);
		}
		catch (const cancelled_promise&)
		{
			Status.store(AsyncStatus::Canceled);
		}
		catch (...)
		{
			Status.store(AsyncStatus::Error);
		}
	}

	void rethrow_if_exception()
	{
		if (Exception)
		{
			std::rethrow_exception(Exception);
		}
	}

	template<typename TExpression>
	[[nodiscard]] constexpr TExpression&& await_transform(TExpression&& Expression)
	{
		if (GetStatus() == AsyncStatus::Canceled)
		{
			throw cancelled_promise();
		}

		return std::forward<TExpression>(Expression);
	}

	void get_return_value() const noexcept {}

	void copy_return_value() const noexcept {}

	[[nodiscard]] AsyncStatus GetStatus() const noexcept { return Status.load(); }

	auto Get()
	{
		AsyncStatus status = Status.load();

		if constexpr (std::is_same_v<TResult, void>)
		{
			if (status == AsyncStatus::Completed)
			{
				return static_cast<TDerived*>(this)->get_return_value();
			}
			// Operation failed
			assert(status == AsyncStatus::Started);
			throw invalid_method_call();
		}
		else
		{
			if (status == AsyncStatus::Completed || status == AsyncStatus::Started)
			{
				return static_cast<TDerived*>(this)->copy_return_value();
			}
			// Operation failed
			assert(status == AsyncStatus::Error || status == AsyncStatus::Canceled);
			std::rethrow_exception(Exception);
		}
	}

	std::atomic<AsyncStatus> Status = AsyncStatus::Started;
	wil::unique_event		 Event;
	std::exception_ptr		 Exception;
};
