#pragma once

struct StaticRigidBody : Component
{
	StaticRigidBody() noexcept = default;
	StaticRigidBody(physx::PxRigidStatic* Actor)
		: Actor(Actor)
	{
	}

	physx::PxRigidStatic* Actor = nullptr;
};
