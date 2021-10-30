#pragma once
#include "PhysicsCommon.h"
#include "PhysicsActor.h"

class PhysicsScene : public PhysicsDeviceChild
{
public:
	PhysicsScene() noexcept = default;
	explicit PhysicsScene(PhysicsDevice* Parent, physx::PxScene* Scene, physx::PxControllerManager* ControllerManager);

	~PhysicsScene();

	NONCOPYABLE(PhysicsScene);
	DEFAULTMOVABLE(PhysicsScene);

	void Reset();

	void AddActor(Entity Entity, physx::PxRigidActor* Actor);

	void AddStaticActor(Entity Entity, const BoxColliderComponent& BoxCollider);
	void AddStaticActor(Entity Entity, const CapsuleColliderComponent& CapsuleCollider);
	void AddStaticActor(Entity Entity, const MeshColliderComponent& MeshCollider);

	void AddDynamicActor(Entity Entity, const BoxColliderComponent& BoxCollider);
	void AddDynamicActor(Entity Entity, const CapsuleColliderComponent& CapsuleCollider);

	void Simulate(float DeltaTime);

private:
	PhysXPtr<physx::PxScene>			 Scene;
	PhysXPtr<physx::PxControllerManager> ControllerManager;
	PhysXPtr<physx::PxMaterial>			 Material;

	std::vector<PhysicsActor> Actors;
};
