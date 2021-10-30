#include "PhysicsScene.h"
#include "PhysicsDevice.h"

using namespace physx;

PhysicsScene::PhysicsScene(PhysicsDevice* Parent, physx::PxScene* Scene, physx::PxControllerManager* ControllerManager)
	: PhysicsDeviceChild(Parent)
	, Scene(Scene)
	, ControllerManager(ControllerManager)
{
	Material = GetParentDevice()->GetPhysics()->createMaterial(0.5f, 0.5f, 0.5f);
}

PhysicsScene::~PhysicsScene()
{
	if (ControllerManager)
	{
		ControllerManager->purgeControllers();
	}
}

void PhysicsScene::Reset()
{
	if (!Actors.empty())
	{
		std::vector<physx::PxActor*> ActorList;
		ActorList.reserve(Actors.size());
		for (const auto& Actor : Actors)
		{
			ActorList.push_back(Actor.GetActor());
		}

		if (!ActorList.empty())
		{
			PxSceneWriteLock Lock(*Scene);
			Scene->removeActors(ActorList.data(), static_cast<physx::PxU32>(ActorList.size()));
		}

		Actors.clear();
	}
}

void PhysicsScene::AddActor(Entity Entity, physx::PxRigidActor* Actor)
{
	PxSceneWriteLock Lock(*Scene);
	Scene->addActor(*Actor);
	Actors.emplace_back(Entity, Actor);
}

void PhysicsScene::AddStaticActor(Entity Entity, const BoxColliderComponent& BoxCollider)
{
	const auto& Transform = Entity.GetComponent<CoreComponent>().Transform;

	PxTransform		  Pose(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));
	PxBoxGeometry	  Geometry(ToPxVec3(BoxCollider.Extents));
	PhysXPtr<PxShape> Shape = GetParentDevice()->GetPhysics()->createShape(Geometry, *Material);

	PxRigidStatic* RigidStatic = GetParentDevice()->GetPhysics()->createRigidStatic(Pose);
	RigidStatic->attachShape(*Shape);

	AddActor(Entity, RigidStatic);
}

void PhysicsScene::AddStaticActor(Entity Entity, const CapsuleColliderComponent& CapsuleCollider)
{
}

void PhysicsScene::AddStaticActor(Entity Entity, const MeshColliderComponent& MeshCollider)
{
	physx::PxPhysics* Physics = GetParentDevice()->GetPhysics();
	physx::PxCooking* Cooking = GetParentDevice()->GetCooking();

	assert(!MeshCollider.Vertices.empty() && !MeshCollider.Indices.empty());
	assert(MeshCollider.Indices.size() % 3 == 0);

	const auto& Transform = Entity.GetComponent<CoreComponent>().Transform;

	PxTransform Pose(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));

	PxCookingParams CookingParams(Physics->getTolerancesScale());
	CookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	CookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	Cooking->setParams(CookingParams);

	PxTriangleMeshDesc Desc = {};
	Desc.points.count		= static_cast<PxU32>(MeshCollider.Vertices.size());
	Desc.points.stride		= sizeof(Vertex);
	Desc.points.data		= MeshCollider.Vertices.data();

	Desc.triangles.count  = static_cast<PxU32>(MeshCollider.Indices.size() / 3);
	Desc.triangles.stride = 3 * sizeof(uint32_t);
	Desc.triangles.data	  = MeshCollider.Indices.data();

	PhysXPtr<PxTriangleMesh> TriangleMesh = Cooking->createTriangleMesh(Desc, Physics->getPhysicsInsertionCallback());
	PxTriangleMeshGeometry	 TriangleMeshGeometry(TriangleMesh.get(), PxMeshScale(ToPxVec3(Transform.Scale)));

	PxRigidStatic* RigidStatic = PxCreateStatic(*Physics, Pose, TriangleMeshGeometry, *Material);
	AddActor(Entity, RigidStatic);
}

void PhysicsScene::AddDynamicActor(Entity Entity, const BoxColliderComponent& BoxCollider)
{
	const auto& Transform = Entity.GetComponent<CoreComponent>().Transform;

	PxTransform		  Pose(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));
	PxBoxGeometry	  Geometry(ToPxVec3(BoxCollider.Extents));
	PhysXPtr<PxShape> Shape = GetParentDevice()->GetPhysics()->createShape(Geometry, *Material);

	PxRigidDynamic* RigidDynamic = GetParentDevice()->GetPhysics()->createRigidDynamic(Pose);
	RigidDynamic->attachShape(*Shape);
	PxRigidBodyExt::updateMassAndInertia(*RigidDynamic, 10.0f);

	AddActor(Entity, RigidDynamic);
}

void PhysicsScene::AddDynamicActor(Entity Entity, const CapsuleColliderComponent& CapsuleCollider)
{
}

void PhysicsScene::Simulate(float DeltaTime)
{
	static double	 ElapsedTime = 0.0;
	constexpr double StepSizeD	 = 1.0 / 60.0;
	constexpr float	 StepSizeF	 = 1.0f / 60.0f;

	ElapsedTime += static_cast<double>(DeltaTime);
	if (ElapsedTime > StepSizeD)
	{
		ElapsedTime = 0.0;

		Scene->simulate(StepSizeF);
		Scene->fetchResults(true);

		for (auto& Actor : Actors)
		{
			Actor.UpdateTransform();
		}
	}
}
