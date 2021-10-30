#pragma once
#include "PhysicsCommon.h"

#include "World/Entity.h"
#include "World/Components.h"

// TODO: Maybe make Entity into Actor and encapsulate physics within it? (like UE)
class PhysicsActor
{
public:
	PhysicsActor() noexcept = default;
	PhysicsActor(Entity Entity, physx::PxRigidActor* Actor)
		: Entity(Entity)
		, Actor(Actor)
	{
	}

	[[nodiscard]] physx::PxRigidActor* GetActor() const noexcept { return Actor.get(); }

	void UpdateTransform();

private:
	Entity						  Entity;
	PhysXPtr<physx::PxRigidActor> Actor;
};
