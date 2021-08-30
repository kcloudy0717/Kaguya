#pragma once

template<typename T>
class RefPtr
{
public:
	using InterfaceType = T;

	template<bool, typename U = void>
	struct EnableIf
	{
	};

	template<typename U>
	struct EnableIf<true, U>
	{
		using type = U;
	};

protected:
	InterfaceType* Ptr;
	template<class U>
	friend class RefPtr;

	void InternalAddRef() const noexcept
	{
		if (Ptr != nullptr)
		{
			Ptr->AddRef();
		}
	}

	unsigned long InternalRelease() noexcept
	{
		unsigned long ref  = 0;
		T*			  temp = Ptr;

		if (temp != nullptr)
		{
			Ptr = nullptr;
			ref = temp->Release();
		}

		return ref;
	}

public:
	RefPtr() noexcept
		: Ptr(nullptr)
	{
	}

	RefPtr(std::nullptr_t) noexcept
		: Ptr(nullptr)
	{
	}

	template<class U>
	RefPtr(U* other) noexcept
		: Ptr(other)
	{
		InternalAddRef();
	}

	RefPtr(const RefPtr& other) noexcept
		: Ptr(other.Ptr)
	{
		InternalAddRef();
	}

	// copy ctor that allows to instanatiate class when U* is convertible to T*
	template<class U>
	RefPtr(
		const RefPtr<U>& other,
		typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
		: Ptr(other.Ptr)

	{
		InternalAddRef();
	}

	RefPtr(RefPtr&& other) noexcept
		: Ptr(nullptr)
	{
		if (this != reinterpret_cast<RefPtr*>(&reinterpret_cast<unsigned char&>(other)))
		{
			Swap(other);
		}
	}

	// Move ctor that allows instantiation of a class when U* is convertible to T*
	template<class U>
	RefPtr(RefPtr<U>&& other, typename std::enable_if<std::is_convertible_v<U*, T*>, void*>::type* = nullptr) noexcept
		: Ptr(other.Ptr)
	{
		other.Ptr = nullptr;
	}

	~RefPtr() noexcept { InternalRelease(); }

	RefPtr& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}

	RefPtr& operator=(T* other) noexcept
	{
		if (Ptr != other)
		{
			RefPtr(other).Swap(*this);
		}
		return *this;
	}

	template<typename U>
	RefPtr& operator=(U* other) noexcept
	{
		RefPtr(other).Swap(*this);
		return *this;
	}

	RefPtr& operator=(const RefPtr& other) noexcept
	{
		if (Ptr != other.Ptr)
		{
			RefPtr(other).Swap(*this);
		}
		return *this;
	}

	template<class U>
	RefPtr& operator=(const RefPtr<U>& other) noexcept
	{
		RefPtr(other).Swap(*this);
		return *this;
	}

	RefPtr& operator=(RefPtr&& other) noexcept
	{
		RefPtr(static_cast<RefPtr&&>(other)).Swap(*this);
		return *this;
	}

	template<class U>
	RefPtr& operator=(RefPtr<U>&& other) noexcept
	{
		RefPtr(static_cast<RefPtr<U>&&>(other)).Swap(*this);
		return *this;
	}

	void Swap(RefPtr&& r) noexcept
	{
		T* tmp = Ptr;
		Ptr	   = r.Ptr;
		r.Ptr  = tmp;
	}

	void Swap(RefPtr& r) noexcept
	{
		T* tmp = Ptr;
		Ptr	   = r.Ptr;
		r.Ptr  = tmp;
	}

	[[nodiscard]] T* Get() const noexcept { return Ptr; }

	operator T*() const { return Ptr; }

	InterfaceType* operator->() const noexcept { return Ptr; }

	T** operator&() { return &Ptr; }

	[[nodiscard]] T* const* GetAddressOf() const noexcept { return &Ptr; }

	[[nodiscard]] T** GetAddressOf() noexcept { return &Ptr; }

	[[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
	{
		InternalRelease();
		return &Ptr;
	}

	T* Detach() noexcept { return std::exchange(Ptr, {}); }

	// Set the pointer while keeping the object's reference count unchanged
	void Attach(InterfaceType* other)
	{
		if (Ptr != nullptr)
		{
			auto ref = Ptr->Release();
			(void)ref;

			// Attaching to the same object only works if duplicate references are being coalesced. Otherwise
			// re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
			assert(ref != 0 || Ptr != other);
		}

		Ptr = other;
	}

	// Create a wrapper around a raw object while keeping the object's reference count unchanged
	static RefPtr<T> Create(T* other)
	{
		RefPtr<T> Ptr;
		Ptr.Attach(other);
		return Ptr;
	}

	unsigned long Reset() { return InternalRelease(); }
};
