#pragma once
#include <coroutine>
#include <iterator>
#include "Async.h"

// A generator should ever only generate objects using co_yield
// user cannot modify the underlying generated object unless they're assigned to another variable
template<typename T>
class Generator;

template<typename T>
struct generator_promise_type final : promise_base<generator_promise_type<T>, T>
{
	using value_type	  = T;
	using pointer		  = value_type*;
	using const_pointer	  = const value_type*;
	using reference		  = value_type&;
	using const_reference = const value_type&;

	auto initial_suspend() noexcept { return std::suspend_always{}; }

	auto final_suspend() noexcept { return std::suspend_always{}; }

	void return_void() noexcept {}

	auto yield_value(const_reference Value) noexcept
	{
		this->Value = &Value;
		return std::suspend_always{};
	}

	const_pointer Value = nullptr;
};

template<typename T>
struct generator_iterator
{
	using iterator_category = std::input_iterator_tag;
	using value_type		= T;
	using difference_type	= std::ptrdiff_t;
	using pointer			= T*;
	using const_pointer		= const T*;
	using reference			= T&;
	using const_reference	= const T&;

	using promise_type	   = generator_promise_type<T>;
	using coroutine_handle = std::coroutine_handle<promise_type>;

	generator_iterator() noexcept = default;
	generator_iterator(coroutine_handle Handle) noexcept
		: Handle(Handle)
	{
	}

	operator bool() const noexcept { return Handle != nullptr; }

	bool operator==(std::default_sentinel_t) const noexcept { return !Handle || Handle.done(); }

	generator_iterator& operator++()
	{
		Handle.resume();
		if (Handle.done())
		{
			Handle.promise().rethrow_if_exception();
		}

		return *this;
	}

	generator_iterator& operator++(int) { return this->operator++(); }

	const_reference operator*() const noexcept { return *Handle.promise().Value; }

	const_pointer operator->() const noexcept { return Handle.promise().Value; }

	coroutine_handle Handle;
};

// ::operator new(size_t, nothrow_t) will be used if allocation is needed
template<typename T>
class Generator
{
public:
	using promise_type	   = generator_promise_type<T>;
	using coroutine_handle = std::coroutine_handle<promise_type>;

	using iterator = generator_iterator<T>;

	Generator() noexcept = default;
	Generator(coroutine_handle Handle) noexcept
		: Handle(Handle)
	{
	}

	~Generator() { InternalDestroy(); }

	Generator(Generator&& Generator) noexcept
		: Handle(std::exchange(Generator.Handle), {})
	{
	}

	Generator& operator=(Generator&& Generator) noexcept
	{
		if (this == &Generator)
		{
			return *this;
		}

		InternalDestroy();
		Handle = std::exchange(Generator.Handle, {});
		return *this;
	}

	Generator(const Generator&) = delete;
	Generator& operator=(const Generator&) = delete;

	iterator begin() { return iterator{ Handle }; }

	std::default_sentinel_t end() noexcept { return std::default_sentinel; }

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
	coroutine_handle Handle = nullptr;
};
