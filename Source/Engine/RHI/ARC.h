#pragma once
#include <cassert>
#include <cstddef>
#include <utility>

namespace RHI
{
	template<typename T>
	concept ArcInterface = requires(T* Ptr)
	{
		Ptr->AddRef();
		Ptr->Release();
	};

	// Auto reference counting
	// Can't name this Arc on windows because it conflicts with a global function :(
	template<ArcInterface T>
	class Arc
	{
	public:
		using InterfaceType = T;

		Arc() noexcept = default;
		Arc(std::nullptr_t) noexcept
			: Ptr(nullptr)
		{
		}

		template<ArcInterface U>
		Arc(U* Ptr) noexcept
			: Ptr(Ptr)
		{
			InternalAddRef();
		}

		Arc(const Arc& Other) noexcept
			: Ptr(Other.Ptr)
		{
			InternalAddRef();
		}

		// copy constructor that allows to instantiate class when U* is convertible to T*
		template<ArcInterface U>
		Arc(const Arc<U>& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr) noexcept
			: Ptr(Other.Ptr)
		{
			InternalAddRef();
		}

		Arc(Arc&& Other) noexcept
			: Ptr(std::exchange(Other.Ptr, nullptr))
		{
		}

		// Move constructor that allows instantiation of a class when U* is convertible to T*
		template<ArcInterface U>
		Arc(Arc<U>&& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr) noexcept
			: Ptr(std::exchange(Other.Ptr, nullptr))
		{
		}

		~Arc() noexcept { InternalRelease(); }

		Arc& operator=(std::nullptr_t) noexcept
		{
			InternalRelease();
			return *this;
		}

		Arc& operator=(T* Ptr) noexcept
		{
			if (this->Ptr != Ptr)
			{
				Arc(Ptr).Swap(*this);
			}
			return *this;
		}

		template<ArcInterface U>
		Arc& operator=(U* Ptr) noexcept
		{
			Arc(Ptr).Swap(*this);
			return *this;
		}

		Arc& operator=(const Arc& Other) noexcept
		{
			if (this->Ptr != Other.Ptr)
			{
				Arc(Other).Swap(*this);
			}
			return *this;
		}

		template<ArcInterface U>
		Arc& operator=(const Arc<U>& Other) noexcept
		{
			Arc(Other).Swap(*this);
			return *this;
		}

		Arc& operator=(Arc&& Other) noexcept
		{
			Arc(std::forward<Arc>(Other)).Swap(*this);
			return *this;
		}

		template<ArcInterface U>
		Arc& operator=(Arc<U>&& Other) noexcept
		{
			Arc(std::forward<Arc<U>>(Other)).Swap(*this);
			return *this;
		}

		void Swap(Arc&& Other) noexcept { std::swap(Ptr, Other.Ptr); }

		void Swap(Arc& Other) noexcept { std::swap(Ptr, Other.Ptr); }

		operator T*() const noexcept { return Ptr; }

		T* Get() const noexcept { return Ptr; }

		InterfaceType* operator->() const noexcept { return Ptr; }

		T* const* GetAddressOf() const noexcept { return &Ptr; }

		T** GetAddressOf() noexcept { return &Ptr; }

		T** ReleaseAndGetAddressOf() noexcept
		{
			InternalRelease();
			return &Ptr;
		}

		T** operator&() noexcept { return ReleaseAndGetAddressOf(); }

		T* Detach() noexcept { return std::exchange(Ptr, nullptr); }

		void Attach(InterfaceType* InterfacePtr) noexcept
		{
			if (Ptr)
			{
				auto Ref = Ptr->Release();
				(void)Ref;
				// Attaching to the same object only works if duplicate references are being coalesced. Otherwise
				// re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
				assert(Ref != 0 || Ptr != InterfacePtr);
			}

			Ptr = InterfacePtr;
		}

		unsigned long Reset() noexcept { return InternalRelease(); }

		// Create a wrapper around a raw object while keeping the object's reference count unchanged
		static Arc<InterfaceType> Create(InterfaceType* InterfacePtr)
		{
			Arc<InterfaceType> Ptr;
			Ptr.Attach(InterfacePtr);
			return Ptr;
		}

	protected:
		void InternalAddRef() const noexcept
		{
			if (Ptr)
			{
				Ptr->AddRef();
			}
		}

		unsigned long InternalRelease() noexcept
		{
			unsigned long RefCount = 0;
			T*			  Temp	   = Ptr;
			if (Temp)
			{
				Ptr		 = nullptr;
				RefCount = Temp->Release();
			}
			return RefCount;
		}

	protected:
		template<ArcInterface U>
		friend class Arc;

		InterfaceType* Ptr = nullptr;
	};
} // namespace RHI
