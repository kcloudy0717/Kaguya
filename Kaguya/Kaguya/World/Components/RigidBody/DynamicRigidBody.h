#pragma once

struct DynamicRigidBody : Component
{
	DynamicRigidBody() noexcept = default;
	DynamicRigidBody(physx::PxRigidDynamic* Actor)
		: Actor(Actor)
	{
	}

	physx::PxRigidDynamic* Actor = nullptr;
};
