#pragma once
#include <unordered_map>

#include <PxPhysicsAPI.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>

#include "PhysXUtility.h"

#include "World/Vertex.h"
#include "World/Entity.h"
#include "World/Components.h"

class PhysicsManager
{
public:
	static void Initialize();

	static void Shutdown();

	static bool Simulate(float dt);

	static physx::PxRigidStatic* AddStaticActorEntity(Entity Entity, const BoxCollider& BoxCollider);
	static physx::PxRigidStatic* AddStaticActorEntity(Entity Entity, const CapsuleCollider& CapsuleCollider);

	static physx::PxRigidDynamic* AddDynamicActorEntity(Entity Entity, const BoxCollider& BoxCollider);
	static physx::PxRigidDynamic* AddDynamicActorEntity(Entity Entity, const CapsuleCollider& CapsuleCollider);

	static physx::PxController* AddControllerForEntity(Entity Entity, const BoxCollider& BoxCollider);
	static physx::PxController* AddControllerForEntity(Entity Entity, const CapsuleCollider& CapsuleCollider);

	static physx::PxRigidStatic* AddGenericActor(Entity Entity, const MeshCollider& MeshCollider);
	static void					 RemoveGenericActor(physx::PxRigidStatic* pActor);

	static void RemoveActor(physx::PxRigidActor* pActor);

private:
	// From PhysX sample
	static void createStack(const physx::PxTransform& t, physx::PxU32 size, physx::PxReal halfExtent);

	static physx::PxRigidDynamic* createDynamic(
		const physx::PxTransform& t,
		const physx::PxGeometry&  geometry,
		const physx::PxVec3&	  velocity = physx::PxVec3(0));

private:
	inline static physx::PxDefaultErrorCallback DefaultErrorCallback;
	inline static physx::PxDefaultAllocator		DefaultAllocatorCallback;

	inline static physx::PxFoundation*		   Foundation = nullptr;
	inline static physx::PxPvd*				   Pvd		  = nullptr;
	inline static physx::PxTolerancesScale	   Scale;
	inline static physx::PxPhysics*			   Physics			  = nullptr;
	inline static physx::PxCooking*			   Cooking			  = nullptr;
	inline static physx::PxCudaContextManager* CudaContextManager = nullptr;

	inline static physx::PxDefaultCpuDispatcher* CpuDispatcher = nullptr;

	inline static physx::PxScene*			  Scene				= nullptr;
	inline static physx::PxControllerManager* ControllerManager = nullptr;
	inline static physx::PxMaterial*		  Material			= nullptr;

	inline static std::unordered_map<physx::PxRigidActor*, Entity> EntityActors;
	inline static std::unordered_map<physx::PxController*, Entity> EntityControllers;

	inline static std::mutex							   GenericActorsMutex;
	inline static std::unordered_set<physx::PxRigidActor*> GenericActors;
};
