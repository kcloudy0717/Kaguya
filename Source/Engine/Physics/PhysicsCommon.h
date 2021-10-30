#pragma once
#include <PxPhysicsAPI.h>

inline physx::PxVec3 ToPxVec3(const DirectX::XMFLOAT3& v)
{
	return physx::PxVec3(v.x, v.y, v.z);
}
inline physx::PxExtendedVec3 ToPxExtendedVec3(const DirectX::XMFLOAT3& v)
{
	return physx::PxExtendedVec3(v.x, v.y, v.z);
}
inline physx::PxQuat ToPxQuat(const DirectX::XMFLOAT4& q)
{
	return physx::PxQuat(q.x, q.y, q.z, q.w);
}

inline DirectX::XMFLOAT3 ToXMFloat3(const physx::PxVec3& v)
{
	return { v.x, v.y, v.z };
}
inline DirectX::XMFLOAT4 ToXMFloat4(const physx::PxQuat& v)
{
	return { v.x, v.y, v.z, v.w };
}

struct UniquePhysXDeleter
{
	template<typename T>
	void operator()(T* Ptr) const
	{
		Ptr->release();
	}
};

template<typename T, class Deleter = UniquePhysXDeleter>
class PhysXPtr : protected std::unique_ptr<T, Deleter>
{
public:
	static_assert(std::is_empty_v<Deleter>, "PhysXPtr doesn't support stateful deleter.");
	using parent  = std::unique_ptr<T, Deleter>;
	using pointer = typename parent::pointer;

	PhysXPtr()
		: parent(nullptr)
	{
	}

	PhysXPtr(T* p)
		: parent(p)
	{
	}

	template<typename UDeleter>
	PhysXPtr(PhysXPtr<T, UDeleter>&& PhysXPtr)
		: parent(PhysXPtr.release())
	{
	}

	template<typename UDeleter>
	PhysXPtr& operator=(PhysXPtr<T, UDeleter>&& PhysXPtr)
	{
		parent::reset(PhysXPtr.release());
		return *this;
	}

	PhysXPtr& operator=(PhysXPtr const&) = delete;
	PhysXPtr(PhysXPtr const&)			 = delete;

	PhysXPtr& operator=(pointer p)
	{
		reset(p);
		return *this;
	}

	PhysXPtr& operator=(std::nullptr_t p)
	{
		reset(p);
		return *this;
	}

	void reset(pointer p = pointer()) { parent::reset(p); }

	void reset(std::nullptr_t p) { parent::reset(p); }

	using parent::get;
	using parent::release;
	using parent::operator->;
	using parent::operator*;
	using parent::operator bool;
};

class PhysicsDevice;

class PhysicsDeviceChild
{
public:
	PhysicsDeviceChild() noexcept
		: Parent(nullptr)
	{
	}
	PhysicsDeviceChild(PhysicsDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	[[nodiscard]] auto GetParentDevice() const noexcept -> PhysicsDevice* { return Parent; }

	void SetParentDevice(PhysicsDevice* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	PhysicsDevice* Parent;
};
