#pragma once
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>

inline static physx::PxVec3 ToPxVec3(const DirectX::XMFLOAT3& v)
{
	return physx::PxVec3(v.x, v.y, v.z);
}
inline static physx::PxExtendedVec3 ToPxExtendedVec3(const DirectX::XMFLOAT3& v)
{
	return physx::PxExtendedVec3(
		static_cast<physx::PxExtended>(v.x),
		static_cast<physx::PxExtended>(v.y),
		static_cast<physx::PxExtended>(v.z));
}
inline static physx::PxQuat ToPxQuat(const DirectX::XMFLOAT4& q)
{
	return physx::PxQuat(q.x, q.y, q.z, q.w);
}

inline static DirectX::XMFLOAT3 ToXMFloat3(const physx::PxVec3& v)
{
	return { v.x, v.y, v.z };
}
inline static DirectX::XMFLOAT4 ToXMFloat4(const physx::PxQuat& v)
{
	return { v.x, v.y, v.z, v.w };
}
