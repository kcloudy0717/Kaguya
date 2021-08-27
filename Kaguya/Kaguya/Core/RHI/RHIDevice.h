#pragma once
#include "RHICommon.h"

class IRHIObject;
class IRHIDevice;
class IRHIDeviceChild;
class IRHIResource;
class IRHIDescriptorTable;

template<typename T>
class RefPtr
{
public:
	typedef T InterfaceType;

	template<bool b, typename U = void>
	struct EnableIf
	{
	};

	template<typename U>
	struct EnableIf<true, U>
	{
		typedef U type;
	};

protected:
	InterfaceType* ptr_;
	template<class U>
	friend class RefPtr;

	void InternalAddRef() const noexcept
	{
		if (ptr_ != nullptr)
		{
			ptr_->AddRef();
		}
	}

	unsigned long InternalRelease() noexcept
	{
		unsigned long ref  = 0;
		T*			  temp = ptr_;

		if (temp != nullptr)
		{
			ptr_ = nullptr;
			ref	 = temp->Release();
		}

		return ref;
	}

public:
	RefPtr() noexcept
		: ptr_(nullptr)
	{
	}

	RefPtr(std::nullptr_t) noexcept
		: ptr_(nullptr)
	{
	}

	template<class U>
	RefPtr(U* other) noexcept
		: ptr_(other)
	{
		InternalAddRef();
	}

	RefPtr(const RefPtr& other) noexcept
		: ptr_(other.ptr_)
	{
		InternalAddRef();
	}

	// copy ctor that allows to instanatiate class when U* is convertible to T*
	template<class U>
	RefPtr(
		const RefPtr<U>& other,
		typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
		: ptr_(other.ptr_)

	{
		InternalAddRef();
	}

	RefPtr(RefPtr&& other) noexcept
		: ptr_(nullptr)
	{
		if (this != reinterpret_cast<RefPtr*>(&reinterpret_cast<unsigned char&>(other)))
		{
			Swap(other);
		}
	}

	// Move ctor that allows instantiation of a class when U* is convertible to T*
	template<class U>
	RefPtr(
		RefPtr<U>&& other,
		typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
		: ptr_(other.ptr_)
	{
		other.ptr_ = nullptr;
	}

	~RefPtr() noexcept { InternalRelease(); }

	RefPtr& operator=(std::nullptr_t) noexcept
	{
		InternalRelease();
		return *this;
	}

	RefPtr& operator=(T* other) noexcept
	{
		if (ptr_ != other)
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
		if (ptr_ != other.ptr_)
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
		T* tmp = ptr_;
		ptr_   = r.ptr_;
		r.ptr_ = tmp;
	}

	void Swap(RefPtr& r) noexcept
	{
		T* tmp = ptr_;
		ptr_   = r.ptr_;
		r.ptr_ = tmp;
	}

	[[nodiscard]] T* Get() const noexcept { return ptr_; }

	operator T*() const { return ptr_; }

	InterfaceType* operator->() const noexcept { return ptr_; }

	T** operator&() { return &ptr_; }

	[[nodiscard]] T* const* GetAddressOf() const noexcept { return &ptr_; }

	[[nodiscard]] T** GetAddressOf() noexcept { return &ptr_; }

	[[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
	{
		InternalRelease();
		return &ptr_;
	}

	T* Detach() noexcept
	{
		T* ptr = ptr_;
		ptr_   = nullptr;
		return ptr;
	}

	// Set the pointer while keeping the object's reference count unchanged
	void Attach(InterfaceType* other)
	{
		if (ptr_ != nullptr)
		{
			auto ref = ptr_->Release();
			(void)ref;

			// Attaching to the same object only works if duplicate references are being coalesced. Otherwise
			// re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
			assert(ref != 0 || ptr_ != other);
		}

		ptr_ = other;
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

class IRHIObject
{
public:
	IRHIObject()		  = default;
	virtual ~IRHIObject() = default;

	unsigned long AddRef() { return ++NumReferences; }

	unsigned long Release()
	{
		unsigned long result = --NumReferences;
		if (result == 0)
		{
			this->~IRHIObject();
		}
		return result;
	}

	template<typename T>
	T* As()
	{
		return static_cast<T*>(this);
	}

	NONCOPYABLE(IRHIObject);
	NONMOVABLE(IRHIObject);

private:
	std::atomic<unsigned long> NumReferences = 1;
};

class IRHIDeviceChild : public IRHIObject
{
public:
	IRHIDeviceChild() noexcept
		: Parent(nullptr)
	{
	}
	IRHIDeviceChild(IRHIDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentDevice() const noexcept -> IRHIDevice* { return Parent; }

	void SetParentDevice(IRHIDevice* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	IRHIDevice* Parent;
};

class IRHIRenderPass : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIDescriptorTable : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIRootSignature : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIResource : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;
};

class IRHIBuffer : public IRHIResource
{
public:
	using IRHIResource::IRHIResource;
};

class IRHITexture : public IRHIResource
{
public:
	using IRHIResource::IRHIResource;
};

class IRHIDescriptorPool : public IRHIDeviceChild
{
public:
	using IRHIDeviceChild::IRHIDeviceChild;

	virtual [[nodiscard]] auto AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle = 0;

	virtual void UpdateDescriptor(const DescriptorHandle& Handle) const = 0;
};

class IRHIDevice : public IRHIObject
{
public:
	virtual [[nodiscard]] RefPtr<IRHIRenderPass>	  CreateRenderPass(const RenderPassDesc& Desc)			 = 0;
	virtual [[nodiscard]] RefPtr<IRHIDescriptorTable> CreateDescriptorTable(const DescriptorTableDesc& Desc) = 0;
	virtual [[nodiscard]] RefPtr<IRHIRootSignature>	  CreateRootSignature(const RootSignatureDesc& Desc)	 = 0;
	virtual [[nodiscard]] RefPtr<IRHIDescriptorPool>  CreateDescriptorPool(const DescriptorPoolDesc& Desc)	 = 0;

	virtual [[nodiscard]] RefPtr<IRHIBuffer>  CreateBuffer(const RHIBufferDesc& Desc)	= 0;
	virtual [[nodiscard]] RefPtr<IRHITexture> CreateTexture(const RHITextureDesc& Desc) = 0;
};

class IRHICommandList : public IRHIDeviceChild
{
public:
};
