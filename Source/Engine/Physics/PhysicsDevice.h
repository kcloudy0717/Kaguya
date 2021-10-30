#pragma once
#include "PhysicsCommon.h"
#include "PhysicsScene.h"

#define PHYSICS_PHYSX_GPU 1

struct PhysicsSettings
{
	float Gravity = -9.81f;
};

class PhysicsDevice
{
public:
	PhysicsDevice();
	~PhysicsDevice();

	void Initialize(const PhysicsSettings& Settings);

	[[nodiscard]] physx::PxPhysics* GetPhysics() const noexcept { return Physics.get(); }
	[[nodiscard]] physx::PxCooking* GetCooking() const noexcept { return Cooking.get(); }

	PhysicsScene CreateScene();

private:
	physx::PxDefaultErrorCallback DefaultErrorCallback;
	physx::PxDefaultAllocator	  DefaultAllocatorCallback;

	PhysXPtr<physx::PxFoundation>		  Foundation;
	PhysXPtr<physx::PxPvd>				  Pvd;
	PhysXPtr<physx::PxPvdTransport>		  PvdTransport;
	physx::PxTolerancesScale			  Scale;
	PhysXPtr<physx::PxPhysics>			  Physics;
	PhysXPtr<physx::PxCooking>			  Cooking;
	PhysXPtr<physx::PxCudaContextManager> CudaContextManager;

	PhysXPtr<physx::PxDefaultCpuDispatcher> CpuDispatcher;

	bool ExtensionInitialized = false;

	PhysicsSettings Settings;
};
