#pragma once

class StaticRigidBodyComponent
{
public:
	StaticRigidBodyComponent() noexcept = default;
	StaticRigidBodyComponent(physx::PxRigidStatic* Actor)
		: Actor(Actor)
	{
	}
	physx::PxRigidStatic* Actor = nullptr;
};

REGISTER_CLASS_ATTRIBUTES(StaticRigidBodyComponent, "StaticRigidBody")
