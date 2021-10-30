#include "PhysicsDevice.h"

// Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#define PVD_HOST "127.0.0.1"

using namespace physx;

PhysicsDevice::PhysicsDevice()
{
}

PhysicsDevice::~PhysicsDevice()
{
	if (ExtensionInitialized)
	{
		PxCloseExtensions();
	}
}

void PhysicsDevice::Initialize(const PhysicsSettings& Settings)
{
	Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
	if (!Foundation)
	{
		throw std::runtime_error("PxCreateFoundation");
	}

#if _DEBUG
	constexpr bool RecordMemoryAllocations = true;
#else
	constexpr bool RecordMemoryAllocations = false;
#endif

	Pvd			 = PxCreatePvd(*Foundation);
	PvdTransport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	Pvd->connect(*PvdTransport.get(), PxPvdInstrumentationFlag::eALL);

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, Scale, RecordMemoryAllocations, Pvd.get());
	if (!Physics)
	{
		throw std::runtime_error("PxCreatePhysics");
	}

	Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Scale));
	if (!Cooking)
	{
		throw std::runtime_error("PxCreateCooking");
	}

#if PHYSICS_PHYSX_GPU
	// Gpu
	PxCudaContextManagerDesc CudaContextManagerDesc;
	CudaContextManager = PxCreateCudaContextManager(*Foundation, CudaContextManagerDesc, PxGetProfilerCallback());
	if (!CudaContextManager)
	{
		throw std::runtime_error("PxCreateCudaContextManager");
	}
#endif

	ExtensionInitialized = PxInitExtensions(*Physics, Pvd.get());
	if (!ExtensionInitialized)
	{
		throw std::runtime_error("PxInitExtensions");
	}

	CpuDispatcher = PxDefaultCpuDispatcherCreate(2);
	if (!CpuDispatcher)
	{
		throw std::runtime_error("PxDefaultCpuDispatcherCreate");
	}

	this->Settings = Settings;
}

PhysicsScene PhysicsDevice::CreateScene()
{
	PxSceneDesc SceneDesc(Physics->getTolerancesScale());
	SceneDesc.gravity		= PxVec3(0.0f, Settings.Gravity, 0.0f);
	SceneDesc.cpuDispatcher = CpuDispatcher.get();
	SceneDesc.filterShader	= PxDefaultSimulationFilterShader;
#if PHYSICS_PHYSX_GPU
	SceneDesc.cudaContextManager = CudaContextManager.get();
	SceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	SceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
#endif

	physx::PxScene*				Scene			  = Physics->createScene(SceneDesc);
	physx::PxControllerManager* ControllerManager = PxCreateControllerManager(*Scene);
	if (PxPvdSceneClient* PvdSceneClient = Scene->getScenePvdClient(); PvdSceneClient)
	{
		PvdSceneClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		PvdSceneClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		PvdSceneClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	return PhysicsScene(this, Scene, ControllerManager);
}

// physx::PxController* PhysicsManager::AddControllerForEntity(Entity Entity, const BoxColliderComponent& BoxCollider)
//{
//	const auto& Transform = Entity.GetComponent<CoreComponent>().Transform;
//
//	PxBoxControllerDesc Desc = {};
//	Desc.halfHeight			 = BoxCollider.Extents.y;
//	Desc.halfSideExtent		 = BoxCollider.Extents.z;
//	Desc.halfForwardExtent	 = BoxCollider.Extents.x;
//
//	PxController* Controller = ControllerManager->createController(Desc);
//	Controller->setPosition(ToPxExtendedVec3(Transform.Position));
//
//	EntityControllers[Controller] = Entity;
//
//	return Controller;
//}
//
// physx::PxController* PhysicsManager::AddControllerForEntity(
//	Entity							Entity,
//	const CapsuleColliderComponent& CapsuleCollider)
//{
//	const auto& Transform = Entity.GetComponent<CoreComponent>().Transform;
//
//	PxCapsuleControllerDesc Desc = {};
//	Desc.material				 = Material;
//	Desc.radius					 = CapsuleCollider.Radius;
//	Desc.height					 = CapsuleCollider.Height;
//
//	PxController* Controller = ControllerManager->createController(Desc);
//	Controller->setPosition(ToPxExtendedVec3(Transform.Position));
//
//	EntityControllers[Controller] = Entity;
//
//	return Controller;
//}
