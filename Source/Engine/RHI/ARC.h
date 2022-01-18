#pragma once
#include <cassert>
#include <cstddef>
#include <utility>

template<typename T>
concept ArcInterface = requires(T* Ptr)
{
	Ptr->AddRef();
	Ptr->Release();
};

// Auto reference counting
// Can't name this Arc on windows because it conflicts with a global function :(
template<ArcInterface T>
class ARC
{
public:
	using InterfaceType = T;

	ARC()
	noexcept = default;
	ARC(std::nullptr_t)
	noexcept
		: Ptr(nullptr)
	{
	}

	template<ArcInterface U>
	ARC(U* Ptr)
	noexcept
		: Ptr(Ptr)
	{
		InternalAddRef();
	}

	ARC(const ARC& Other)
	noexcept
		: Ptr(Other.Ptr)
	{
		InternalAddRef();
	}

	// copy constructor that allows to instantiate class when U* is convertible to T*
	template<ArcInterface U>
	ARC(const ARC<U>& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr)
	noexcept
		: Ptr(Other.Ptr)
	{
		InternalAddRef();
	}

	ARC(ARC&& Other)
	noexcept
		: Ptr(std::exchange(Other.Ptr, nullptr))
	{
	}

	// Move constructor that allows instantiation of a class when U* is convertible to T*
	template<ArcInterface U>
	ARC(ARC<U>&& Other, std::enable_if_t<std::is_convertible_v<U*, T*>, void*>* = nullptr)
	noexcept
		: Ptr(std::exchange(Other.Ptr, nullptr))
	{
	}

	~ARC() noexcept { InternalRelease(); }

	ARC& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}

	ARC& operator=(T* Ptr) noexcept
	{
		if (this->Ptr != Ptr)
		{
			ARC(Ptr).Swap(*this);
		}
		return *this;
	}

	template<ArcInterface U>
	ARC& operator=(U* Ptr) noexcept
	{
		ARC(Ptr).Swap(*this);
		return *this;
	}

	ARC& operator=(const ARC& Other) noexcept
	{
		if (this->Ptr != Other.Ptr)
		{
			ARC(Other).Swap(*this);
		}
		return *this;
	}

	template<ArcInterface U>
	ARC& operator=(const ARC<U>& Other) noexcept
	{
		ARC(Other).Swap(*this);
		return *this;
	}

	ARC& operator=(ARC&& Other) noexcept
	{
		ARC(std::forward<ARC>(Other)).Swap(*this);
		return *this;
	}

	template<ArcInterface U>
	ARC& operator=(ARC<U>&& Other) noexcept
	{
		ARC(std::forward<ARC<U>>(Other)).Swap(*this);
		return *this;
	}

	void Swap(ARC&& Other) noexcept { std::swap(Ptr, Other.Ptr); }

	void Swap(ARC& Other) noexcept { std::swap(Ptr, Other.Ptr); }

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
	static ARC<InterfaceType> Create(InterfaceType* InterfacePtr)
	{
		ARC<InterfaceType> Ptr;
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
	friend class ARC;

	InterfaceType* Ptr = nullptr;
};
