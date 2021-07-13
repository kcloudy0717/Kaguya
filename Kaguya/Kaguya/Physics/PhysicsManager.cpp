#include "PhysicsManager.h"

// Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#define PVD_HOST "127.0.0.1"

using namespace physx;

template<typename T>
void PxSafeRelease(T*& ptr)
{
	if (ptr)
	{
		ptr->release();
		ptr = nullptr;
	}
}

void PhysicsManager::Initialize()
{
	Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
	if (!Foundation)
	{
		throw std::runtime_error("PxCreateFoundation");
	}

	constexpr bool RecordMemoryAllocations = true;

	Pvd						  = PxCreatePvd(*Foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, Scale, RecordMemoryAllocations, Pvd);
	if (!Physics)
	{
		throw std::runtime_error("PxCreatePhysics");
	}

	Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Scale));
	if (!Cooking)
	{
		throw std::runtime_error("PxCreateCooking");
	}

	// Gpu
	PxCudaContextManagerDesc cudaContextManagerDesc;
	CudaContextManager = PxCreateCudaContextManager(*Foundation, cudaContextManagerDesc, PxGetProfilerCallback());
	if (!CudaContextManager)
	{
		throw std::runtime_error("PxCreateCudaContextManager");
	}

	if (!PxInitExtensions(*Physics, Pvd))
	{
		throw std::runtime_error("PxInitExtensions");
	}

	CpuDispatcher = PxDefaultCpuDispatcherCreate(2);
	if (!CpuDispatcher)
	{
		throw std::runtime_error("PxDefaultCpuDispatcherCreate");
	}

	PxSceneDesc SceneDesc(Physics->getTolerancesScale());
	SceneDesc.gravity			 = PxVec3(0.0f, -9.81f, 0.0f);
	SceneDesc.cpuDispatcher		 = CpuDispatcher;
	SceneDesc.filterShader		 = PxDefaultSimulationFilterShader;
	SceneDesc.cudaContextManager = CudaContextManager;
	SceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	SceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;

	Scene = Physics->createScene(SceneDesc);

	ControllerManager = PxCreateControllerManager(*Scene);
	if (!ControllerManager)
	{
		throw std::runtime_error("PxCreateControllerManager");
	}

	Material = Physics->createMaterial(0.5f, 0.5f, 0.5f);

	// PxRigidStatic* groundPlane = PxCreatePlane(*Physics, PxPlane(0, 1, 0, 0), *Material);
	// Scene->addActor(*groundPlane);

	PxPvdSceneClient* pPvdClient = Scene->getScenePvdClient();
	if (pPvdClient)
	{
		pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pPvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
}

void PhysicsManager::Shutdown()
{
	ControllerManager->purgeControllers();
	PxSafeRelease(ControllerManager);
	PxSafeRelease(Scene);
	PxSafeRelease(CpuDispatcher);
	PxCloseExtensions();
	PxSafeRelease(CudaContextManager);
	PxSafeRelease(Cooking);
	PxSafeRelease(Physics);
	if (Pvd)
	{
		PxPvdTransport* pPvdTransport = Pvd->getTransport();
		PxSafeRelease(Pvd);
		PxSafeRelease(pPvdTransport);
	}
	PxSafeRelease(Foundation);
}

bool PhysicsManager::Simulate(float dt)
{
	bool			   AnySimulated = false;
	static float	   Accumulator	= 0;
	static const float StepSize		= 1.0f / 60.0f;

	Accumulator += dt;
	if (Accumulator < StepSize)
	{
		return false;
	}

	Accumulator -= StepSize;

	Scene->simulate(StepSize);
	Scene->fetchResults(true);

	// Update entity transform for rigid body
	PxSceneReadLock	 _(*Scene);
	PxActorTypeFlags actorTypeFlags = PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC;
	PxU32			 nbActors		= Scene->getNbActors(actorTypeFlags);
	if (nbActors > 0)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		Scene->getActors(actorTypeFlags, reinterpret_cast<PxActor**>(actors.data()), nbActors);

		for (auto actor : actors)
		{
			const bool sleeping = actor->is<PxRigidDynamic>() ? actor->is<PxRigidDynamic>()->isSleeping() : false;
			if (sleeping)
			{
				continue;
			}

			if (auto iter = EntityActors.find(actor); iter != EntityActors.end())
			{
				AnySimulated	= true;
				auto& transform = iter->second.GetComponent<Transform>();

				PxTransform actorPose = actor->getGlobalPose();
				transform.Position	  = ToXMFloat3(actorPose.p);
				transform.Orientation = ToXMFloat4(actorPose.q);
			}
		}
	}

	return AnySimulated;
}

physx::PxRigidStatic* PhysicsManager::AddStaticActorEntity(Entity Entity, const BoxCollider& BoxCollider)
{
	const auto&	  transform = Entity.GetComponent<Transform>();
	PxTransform	  pxTransform(ToPxVec3(transform.Position), ToPxQuat(transform.Orientation));
	PxBoxGeometry pxBoxGeometry(ToPxVec3(BoxCollider.Extents));
	PxShape*	  shape = Physics->createShape(pxBoxGeometry, *Material);

	PxRigidStatic* body = Physics->createRigidStatic(pxTransform);
	body->attachShape(*shape);

	EntityActors[body] = Entity;
	PxSceneWriteLock _(*Scene);
	Scene->addActor(*body);
	return body;
}

physx::PxRigidStatic* PhysicsManager::AddStaticActorEntity(Entity Entity, const CapsuleCollider& CapsuleCollider)
{
	// TODO: Implement;
	return nullptr;
}

physx::PxRigidDynamic* PhysicsManager::AddDynamicActorEntity(Entity Entity, const BoxCollider& BoxCollider)
{
	const auto&	  transform = Entity.GetComponent<Transform>();
	PxTransform	  pxTransform(ToPxVec3(transform.Position), ToPxQuat(transform.Orientation));
	PxBoxGeometry pxBoxGeometry(ToPxVec3(BoxCollider.Extents));
	PxShape*	  shape = Physics->createShape(pxBoxGeometry, *Material);

	PxRigidDynamic* body = Physics->createRigidDynamic(pxTransform);
	body->attachShape(*shape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

	EntityActors[body] = Entity;
	PxSceneWriteLock _(*Scene);
	Scene->addActor(*body);
	return body;
}

physx::PxRigidDynamic* PhysicsManager::AddDynamicActorEntity(Entity Entity, const CapsuleCollider& CapsuleCollider)
{
	// TODO: Implement;
	return nullptr;
}

physx::PxController* PhysicsManager::AddControllerForEntity(Entity Entity, const BoxCollider& BoxCollider)
{
	PxBoxControllerDesc desc = {};
	desc.halfHeight			 = BoxCollider.Extents.y;
	desc.halfSideExtent		 = BoxCollider.Extents.z;
	desc.halfForwardExtent	 = BoxCollider.Extents.x;

	PxController* controller = ControllerManager->createController(desc);
	const auto&	  position	 = Entity.GetComponent<Transform>().Position;
	controller->setPosition({ position.x, position.y, position.z });

	EntityControllers[controller] = Entity;

	return controller;
}

physx::PxController* PhysicsManager::AddControllerForEntity(Entity Entity, const CapsuleCollider& CapsuleCollider)
{
	PxCapsuleControllerDesc desc = {};
	desc.material				 = Material;

	desc.radius = CapsuleCollider.Radius;
	desc.height = CapsuleCollider.Height;

	PxController* controller = ControllerManager->createController(desc);
	const auto&	  position	 = Entity.GetComponent<Transform>().Position;
	controller->setPosition({ position.x, position.y, position.z });

	EntityControllers[controller] = Entity;

	return controller;
}

physx::PxRigidStatic* PhysicsManager::AddGenericActor(const Transform& Transform, const MeshCollider& MeshCollider)
{
	assert(!MeshCollider.Vertices.empty() && !MeshCollider.Indices.empty());
	assert(MeshCollider.Indices.size() % 3 == 0);

	PxTransform pxTransform(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));

	PxCookingParams params(Scale);
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	Cooking->setParams(params);

	PxTriangleMeshDesc meshDesc = {};
	meshDesc.points.count		= static_cast<PxU32>(MeshCollider.Vertices.size());
	meshDesc.points.stride		= sizeof(Vertex);
	meshDesc.points.data		= MeshCollider.Vertices.data();

	meshDesc.triangles.count  = static_cast<PxU32>(MeshCollider.Indices.size() / 3);
	meshDesc.triangles.stride = 3 * sizeof(uint32_t);
	meshDesc.triangles.data	  = MeshCollider.Indices.data();

	PxTriangleMesh*		   triangleMesh = Cooking->createTriangleMesh(meshDesc, Physics->getPhysicsInsertionCallback());
	PxTriangleMeshGeometry geom(triangleMesh);
	PxRigidStatic*		   pStatic = PxCreateStatic(*Physics, pxTransform, geom, *Material);

	{
		std::scoped_lock _(GenericActorsMutex);
		GenericActors.insert(pStatic);
	}
	{
		PxSceneWriteLock _(*Scene);
		Scene->addActor(*pStatic);
		triangleMesh->release();
	}
	return pStatic;
}

void PhysicsManager::RemoveGenericActor(physx::PxRigidStatic* pActor)
{
	if (pActor)
	{
		{
			std::scoped_lock _(GenericActorsMutex);
			GenericActors.erase(pActor);
		}
		{
			PxSceneWriteLock _(*Scene);
			Scene->removeActor(*pActor);
			pActor->release();
		}
	}
}

void PhysicsManager::RemoveActor(physx::PxRigidActor* pActor)
{
	if (pActor)
	{
		EntityActors.erase(pActor);
		PxSceneWriteLock _(*Scene);
		Scene->removeActor(*pActor);
	}
}

void PhysicsManager::createStack(const physx::PxTransform& t, physx::PxU32 size, physx::PxReal halfExtent)
{
	PxShape* shape = Physics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *Material);
	for (PxU32 i = 0; i < size; i++)
	{
		for (PxU32 j = 0; j < size - i; j++)
		{
			PxTransform		localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = Physics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			Scene->addActor(*body);
		}
	}
	shape->release();
}

physx::PxRigidDynamic* PhysicsManager::createDynamic(
	const physx::PxTransform& t,
	const physx::PxGeometry&  geometry,
	const physx::PxVec3&	  velocity /*= physx::PxVec3(0)*/)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*Physics, t, geometry, *Material, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	Scene->addActor(*dynamic);
	return dynamic;
}
