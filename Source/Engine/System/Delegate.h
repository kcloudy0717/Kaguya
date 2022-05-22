﻿#pragma once
#include <cassert>
#include <mutex>
#include "Types.h"

// Delegate class can be used as an alternative to std::function, delegates are flexible
// It can be binded to any types of subroutines
//
// https://skypjack.github.io/2019-01-25-delegate-revised/
// https://simoncoenen.com/blog/programming/CPP_Delegates

namespace Kaguya
{
	namespace Internal
	{
		template<u32 MaxStackSizeInBytes>
		class InlineAllocator
		{
		public:
			InlineAllocator() noexcept
				: Buffer{}
				, SizeInBytes(0)
			{
			}
			InlineAllocator(InlineAllocator&& InlineAllocator) noexcept
				: SizeInBytes(std::exchange(InlineAllocator.SizeInBytes, 0))
			{
				if (HasHeapAllocation())
				{
					Ptr = std::exchange(InlineAllocator.Ptr, {});
				}
				else
				{
					memcpy(Buffer, InlineAllocator.Buffer, SizeInBytes);
				}
				Destructor = std::exchange(InlineAllocator.Destructor, {});
			}
			InlineAllocator& operator=(InlineAllocator&& InlineAllocator) noexcept
			{
				if (this == &InlineAllocator)
				{
					return *this;
				}

				Free();
				SizeInBytes = std::exchange(InlineAllocator.SizeInBytes, 0);
				if (HasHeapAllocation())
				{
					Ptr = std::exchange(InlineAllocator.Ptr, {});
				}
				else
				{
					memcpy(Buffer, InlineAllocator.Buffer, SizeInBytes);
				}
				Destructor = std::exchange(InlineAllocator.Destructor, {});
				return *this;
			}
			~InlineAllocator()
			{
				Free();
			}

			InlineAllocator(const InlineAllocator&) = delete;
			InlineAllocator& operator=(const InlineAllocator&) = delete;

			[[nodiscard]] std::byte* GetAllocation()
			{
				if (HasAllocation())
				{
					return HasHeapAllocation() ? Ptr : Buffer;
				}
				return nullptr;
			}
			[[nodiscard]] const std::byte* GetAllocation() const
			{
				if (HasAllocation())
				{
					return HasHeapAllocation() ? Ptr : Buffer;
				}
				return nullptr;
			}

			[[nodiscard]] u32  GetSize() const noexcept { return SizeInBytes; }
			[[nodiscard]] bool HasAllocation() const noexcept { return SizeInBytes > 0; }
			[[nodiscard]] bool HasHeapAllocation() const noexcept { return SizeInBytes > MaxStackSizeInBytes; }

			std::byte* Allocate(u32 SizeInBytes)
			{
				if (this->SizeInBytes != SizeInBytes)
				{
					Free();
					this->SizeInBytes = SizeInBytes;
					if (SizeInBytes > MaxStackSizeInBytes)
					{
						Ptr = new std::byte[SizeInBytes];
						return Ptr;
					}
				}
				return Buffer;
			}

			template<typename T>
			T* Allocate()
			{
				std::byte* Address = Allocate(sizeof(T));
				Destructor		   = &DestructorCallback<T>;
				return reinterpret_cast<T*>(Address);
			}

			void Free()
			{
				if (HasAllocation() && Destructor)
				{
					Destructor(GetAllocation());
					Destructor = nullptr;
				}
				if (HasHeapAllocation())
				{
					delete[] Ptr;
					Ptr = nullptr;
				}
				SizeInBytes = 0;
			}

		private:
			template<typename T>
			static void DestructorCallback(void* Object)
			{
				static_cast<T*>(Object)->~T();
			}

		private:
			union
			{
				std::byte  Buffer[MaxStackSizeInBytes];
				std::byte* Ptr;
			};
			u32 SizeInBytes;
			void (*Destructor)(void*) = nullptr;
		};
	} // namespace Internal
} // namespace Kaguya

struct DelegateHandle
{
	[[nodiscard]] auto operator<=>(const DelegateHandle&) const noexcept = default;

	void Reset()
	{
		Context = nullptr;
		Id		= 0xDEADBEEF;
	}

	[[nodiscard]] bool IsValid() const noexcept { return Context != nullptr && Id != 0xDEADBEEF; }

	void* Context = nullptr;
	u32	  Id	  = 0xDEADBEEF;
};

template<typename T>
class Delegate;

template<typename TRet, typename... TArgs>
class Delegate<TRet(TArgs...)>
{
public:
	using ProxyType = TRet (*)(void*, TArgs&&...);

	Delegate() noexcept = default;
	Delegate(Delegate&& Delegate) noexcept
		: Allocator(std::exchange(Delegate.Allocator, {}))
		, Object(Allocator.GetAllocation())
		, Proxy(std::exchange(Delegate.Proxy, {}))
	{
	}
	Delegate& operator=(Delegate&& Delegate) noexcept
	{
		if (this == &Delegate)
		{
			return *this;
		}

		Allocator = std::exchange(Delegate.Allocator, {});
		// Object cannot be safely exchanged, it must use the Allocator's memory allocation
		Object = Allocator.GetAllocation();
		// Proxy can be safely exchanged
		Proxy = std::exchange(Delegate.Proxy, {});
		return *this;
	}

	Delegate(const Delegate&) = delete;
	Delegate& operator=(const Delegate&) = delete;

	Delegate(std::nullptr_t) noexcept { Reset(); }

	template<typename T>
	requires std::invocable<T, TArgs...> Delegate(T&& Lambda)
	{
		using DecayedType = std::decay_t<T>;

		if (Allocator.HasAllocation())
		{
			Allocator.Free();
		}

		Object = Allocator.Allocate<DecayedType>();
		Proxy  = FunctionProxy<DecayedType>;

		new (Object) DecayedType(std::forward<DecayedType>(Lambda));
	}

	template<typename T>
	requires std::invocable<T, TArgs...> Delegate& operator=(T&& Lambda)
	{
		using DecayedType = std::decay_t<T>;

		if (Allocator.HasAllocation())
		{
			Allocator.Free();
		}

		Object = Allocator.Allocate<DecayedType>();
		Proxy  = FunctionProxy<DecayedType>;

		new (Object) DecayedType(std::forward<DecayedType>(Lambda));

		return *this;
	}

	template<TRet (*Function)(TArgs...)>
	static Delegate Construct() noexcept
	{
		return { nullptr, FunctionProxy<Function> };
	}

	template<typename T>
	static Delegate Construct(T&& Lambda)
	{
		return std::forward<T>(Lambda);
	}

	template<auto Method, typename T>
	static Delegate Construct(T* Object) noexcept
	{
		return { Object, FunctionProxy<T, Method> };
	}

	template<auto Method, typename T>
	static Delegate Construct(const T* Object) noexcept
	{
		return { const_cast<T*>(Object), FunctionProxy<T, Method> };
	}

	template<typename T, TRet (T::*Method)(TArgs...)>
	static Delegate Construct(T& Object) noexcept
	{
		return { &Object, FunctionProxy<T, Method> };
	}

	template<typename T, TRet (T::*Method)(TArgs...) const>
	static Delegate Construct(const T& Object) noexcept
	{
		return { const_cast<T*>(&Object), FunctionProxy<T, Method> };
	}

	static Delegate Construct(TRet (*Function)(TArgs...)) { return Function; }

	template<auto Method, typename T>
	void Bind(T* Object)
	{
		*this = Construct<Method, T>(Object);
	}

	template<auto Method, typename T>
	void Bind(const T* Object)
	{
		*this = Construct<Method, T>(Object);
	}

	template<auto Method, typename T>
	void Bind(T& Object)
	{
		*this = Construct<Method, T>(Object);
	}

	template<auto Method, typename T>
	void Bind(const T& Object)
	{
		*this = Construct<Method, T>(Object);
	}

	void Reset() noexcept { Proxy = nullptr; }

	bool operator==(const Delegate& Delegate) const noexcept
	{
		return Object == Delegate.Object && (Proxy == Delegate.Proxy);
	}
	bool operator!=(const Delegate& Delegate) const noexcept { return !(*this == Delegate); }

	bool operator==(std::nullptr_t) const noexcept { return !Proxy; }
	bool operator!=(std::nullptr_t) const noexcept { return Proxy; }

	explicit operator bool() const noexcept { return Proxy; }

	TRet operator()(TArgs... Args) const
	{
		assert(static_cast<bool>(*this) && "Invalid proxy");
		return Proxy(Object, std::forward<TArgs>(Args)...);
	}

private:
	Delegate(void* Object, ProxyType Proxy) noexcept
		: Object(Object)
		, Proxy(Proxy)
	{
	}

	// Function
	template<TRet (*Function)(TArgs...)>
	static TRet FunctionProxy(void*, TArgs&&... Args)
	{
		return Function(std::forward<TArgs>(Args)...);
	}

	// Lambda
	template<typename T>
	static TRet FunctionProxy(void* Object, TArgs&&... Args)
	{
		return (*static_cast<T*>(Object))(std::forward<TArgs>(Args)...);
	}

	// Method
	template<typename T, TRet (T::*Method)(TArgs...)>
	static TRet FunctionProxy(void* Object, TArgs&&... Args)
	{
		return (static_cast<T*>(Object)->*Method)(std::forward<TArgs>(Args)...);
	}

	// Const Method
	template<typename T, TRet (T::*Method)(TArgs...) const>
	static TRet FunctionProxy(void* Object, TArgs&&... Args)
	{
		return (static_cast<const T*>(Object)->*Method)(std::forward<TArgs>(Args)...);
	}

private:
	Kaguya::Internal::InlineAllocator<64> Allocator;
	void*								  Object = nullptr;
	ProxyType							  Proxy	 = nullptr;
};

template<typename... TArgs>
class MulticastDelegate
{
public:
	using DelegateType = Delegate<void(TArgs...)>;

	MulticastDelegate(bool ThreadSafe = false)
		: OptMutex(ThreadSafe ? std::make_unique<std::mutex>() : nullptr)
	{
	}

	DelegateHandle Add(DelegateType&& Delegate)
	{
		auto Lock = OptMutex ? std::unique_lock(*OptMutex) : std::unique_lock<std::mutex>();

		DelegateHandlePair& Pair = Delegates.emplace_back();
		Pair.Handle				 = { this, DelegateId++ };
		Pair.Delegate			 = std::move(Delegate);
		return Pair.Handle;
	}

	void Remove(DelegateHandle& Handle)
	{
		auto Lock = OptMutex ? std::unique_lock(*OptMutex) : std::unique_lock<std::mutex>();

		assert(Handle.IsValid());
		for (size_t i = 0; i < Delegates.size(); ++i)
		{
			if (Delegates[i].Handle == Handle)
			{
				std::swap(Delegates[i], Delegates[Delegates.size() - 1]);
				Delegates.pop_back();
				Handle.Reset();
				break;
			}
		}
	}

	template<typename T>
	DelegateHandle operator+=(T&& Lambda)
	{
		return Add(DelegateType::Construct(std::forward<T>(Lambda)));
	}

	DelegateHandle operator+=(DelegateType&& Delegate) { return Add(std::forward<DelegateType>(Delegate)); }

	void operator()(TArgs... Args) const
	{
		auto Lock = OptMutex ? std::unique_lock(*OptMutex) : std::unique_lock<std::mutex>();
		for (const auto& Delegate : Delegates)
		{
			Delegate.Delegate(std::forward<TArgs>(Args)...);
		}
	}

private:
	struct DelegateHandlePair
	{
		DelegateHandle Handle;
		DelegateType   Delegate;
	};

	u32								DelegateId = 0;
	std::vector<DelegateHandlePair> Delegates;
	std::unique_ptr<std::mutex>		OptMutex;
};
