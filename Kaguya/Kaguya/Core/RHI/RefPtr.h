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
	RefPtr(U* Other) noexcept
		: Ptr(Other)
	{
		InternalAddRef();
	}

	RefPtr(const RefPtr& Other) noexcept
		: Ptr(Other.Ptr)
	{
		InternalAddRef();
	}

	// copy ctor that allows to instanatiate class when U* is convertible to T*
	template<class U>
	RefPtr(const RefPtr<U>& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr) noexcept
		: Ptr(Other.Ptr)

	{
		InternalAddRef();
	}

	RefPtr(RefPtr&& Other) noexcept
		: Ptr(nullptr)
	{
		if (this != reinterpret_cast<RefPtr*>(&reinterpret_cast<unsigned char&>(Other)))
		{
			Swap(Other);
		}
	}

	// Move ctor that allows instantiation of a class when U* is convertible to T*
	template<class U>
	RefPtr(RefPtr<U>&& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr) noexcept
		: Ptr(Other.Ptr)
	{
		Other.Ptr = nullptr;
	}

	~RefPtr() noexcept { InternalRelease(); }

	RefPtr& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}

	RefPtr& operator=(T* Other) noexcept
	{
		if (Ptr != Other)
		{
			RefPtr(Other).Swap(*this);
		}
		return *this;
	}

	template<typename U>
	RefPtr& operator=(U* Other) noexcept
	{
		RefPtr(Other).Swap(*this);
		return *this;
	}

	RefPtr& operator=(const RefPtr& Other) noexcept
	{
		if (Ptr != Other.Ptr)
		{
			RefPtr(Other).Swap(*this);
		}
		return *this;
	}

	template<class U>
	RefPtr& operator=(const RefPtr<U>& Other) noexcept
	{
		RefPtr(Other).Swap(*this);
		return *this;
	}

	RefPtr& operator=(RefPtr&& Other) noexcept
	{
		RefPtr(static_cast<RefPtr&&>(Other)).Swap(*this);
		return *this;
	}

	template<class U>
	RefPtr& operator=(RefPtr<U>&& Other) noexcept
	{
		RefPtr(static_cast<RefPtr<U>&&>(Other)).Swap(*this);
		return *this;
	}

	void Swap(RefPtr&& RefPtr) noexcept
	{
		T* Temp	   = Ptr;
		Ptr		   = RefPtr.Ptr;
		RefPtr.Ptr = Temp;
	}

	void Swap(RefPtr& RefPtr) noexcept
	{
		T* Temp	   = Ptr;
		Ptr		   = RefPtr.Ptr;
		RefPtr.Ptr = Temp;
	}

	[[nodiscard]] T* Get() const noexcept { return Ptr; }

	operator T*() const { return Ptr; }

	InterfaceType* operator->() const noexcept { return Ptr; }

	[[nodiscard]] T* const* GetAddressOf() const noexcept { return &Ptr; }

	[[nodiscard]] T** GetAddressOf() noexcept { return &Ptr; }

	[[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
	{
		InternalRelease();
		return &Ptr;
	}

	T* Detach() noexcept { return std::exchange(Ptr, {}); }

	// Set the pointer while keeping the object's reference count unchanged
	void Attach(InterfaceType* InterfacePtr)
	{
		if (Ptr != nullptr)
		{
			auto Ref = Ptr->Release();
			(void)Ref;
			// Attaching to the same object only works if duplicate references are being coalesced. Otherwise
			// re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
			assert(Ref != 0 || Ptr != InterfacePtr);
		}

		Ptr = InterfacePtr;
	}

	// Create a wrapper around a raw object while keeping the object's reference count unchanged
	static RefPtr<T> Create(T* InterfacePtr)
	{
		RefPtr<T> Ptr;
		Ptr.Attach(InterfacePtr);
		return Ptr;
	}

	unsigned long Reset() { return InternalRelease(); }
};
