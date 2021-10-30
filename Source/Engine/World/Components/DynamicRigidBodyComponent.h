#pragma once

class DynamicRigidBodyComponent
{
public:
	DynamicRigidBodyComponent() noexcept = default;
	DynamicRigidBodyComponent(physx::PxRigidDynamic* Actor)
		: Actor(Actor)
	{
	}
	physx::PxRigidDynamic* Actor = nullptr;
};

REGISTER_CLASS_ATTRIBUTES(DynamicRigidBodyComponent, "DynamicRigidBody")
