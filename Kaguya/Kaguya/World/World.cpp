#include "World.h"

#include "Entity.h"
#include "Physics/PhysicsManager.h"
#include "Graphics/AssetManager.h"

#include "Scripts/Player.script.h"

static const char* DefaultEntityName = "GameObject";

auto World::CreateEntity(std::string_view Name /*= {}*/) -> Entity
{
	Entity Entity = { Registry.create(), this };
	Entity.AddComponent<Tag>(Name.empty() ? DefaultEntityName : Name);
	Entity.AddComponent<Transform>();
	return Entity;
}

auto World::GetMainCamera() -> Entity
{
	auto View = Registry.view<Camera>();
	for (entt::entity Handle : View)
	{
		const auto& Component = View.get<Camera>(Handle);
		if (Component.Main)
		{
			return Entity(Handle, this);
		}
	}
	return Entity(entt::null, this);
}

auto World::GetEntityByTag(std::string_view Name) -> Entity
{
	auto View = Registry.view<Tag>();
	for (entt::entity Handle : View)
	{
		const auto& Component = View.get<Tag>(Handle);
		if (Component.Name == Name)
		{
			return Entity(Handle, this);
		}
	}
	return Entity(entt::null, this);
}

void World::Clear(bool bAddDefaultEntities /*= true*/)
{
	WorldState = EWorldState_Update;
	Registry.clear();
	ActiveCamera = nullptr;
	if (bAddDefaultEntities)
	{
		AddDefaultEntities();
	}
}

void World::DestroyEntity(Entity Entity)
{
	Registry.destroy(Entity);
}

void World::Update(float dt)
{
	ResolveComponentDependencies();
	UpdateScripts(dt);
	UpdatePhysics(dt);
}

void World::ResolveComponentDependencies()
{
	Registry.view<Transform, Camera>().each(
		[&](auto&& Transform, auto&& Camera)
		{
			Camera.pTransform = &Transform;

			if (Camera.Dirty)
			{
				Camera.Dirty = false;
				WorldState |= EWorldState_Update;
			}
		});

	// Update all mesh filters
	Registry.view<MeshFilter>().each(
		[](auto&& MeshFilter)
		{
			MeshFilter.Mesh = AssetManager::GetMeshCache().Load(MeshFilter.Key);
			if (MeshFilter.Mesh)
			{
				MeshFilter.Path =
					std::filesystem::relative(MeshFilter.Mesh->Metadata.Path, Application::ExecutableDirectory)
						.string();
			}
			else
			{
				MeshFilter.Path = "<unknown>";
			}
		});

	// Update all mesh renderers
	Registry.view<MeshFilter, MeshRenderer>().each(
		[](auto&& MeshFilter, auto&& MeshRenderer)
		{
			MeshRenderer.pMeshFilter = &MeshFilter;

			auto Texture = AssetManager::GetImageCache().Load(MeshRenderer.Material.Albedo.Key);

			if (Texture)
			{
				MeshRenderer.Material.Albedo.Path =
					std::filesystem::relative(Texture->Metadata.Path, Application::ExecutableDirectory).string();
				MeshRenderer.Material.TextureIndices[0] = Texture->SRV.GetIndex();
			}
			else
			{
				MeshRenderer.Material.Albedo.Path = "<unknown>";
			}
		});
}

void World::UpdateScripts(float dt)
{
	Registry.view<Transform, StaticRigidBody>().each(
		[&](auto&& Transform, auto&& StaticRigidBody)
		{
			physx::PxTransform pxTransform(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));
			StaticRigidBody.Actor->setGlobalPose(pxTransform);
		});

	Registry.view<Transform, DynamicRigidBody>().each(
		[&](auto&& Transform, auto&& DynamicRigidBody)
		{
			physx::PxTransform pxTransform(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));
			DynamicRigidBody.Actor->setGlobalPose(pxTransform);
		});

	Registry.view<NativeScript>().each(
		[&](auto Handle, NativeScript& NativeScript)
		{
			if (!NativeScript.Instance)
			{
				NativeScript.Instance		  = NativeScript.InstantiateScript();
				NativeScript.Instance->Entity = Entity{ Handle, this };
				NativeScript.Instance->OnCreate();
			}

			NativeScript.Instance->OnUpdate(dt);
		});
}

void World::UpdatePhysics(float dt)
{
	if (PhysicsManager::Simulate(dt))
	{
		WorldState |= EWorldState_Update;
	}
}

void World::AddDefaultEntities()
{
	Entity MainCamera = CreateEntity("Main Camera");
	MainCamera.AddComponent<Camera>();
}

template<typename T>
void World::OnComponentAdded(Entity Entity, T& Component)
{
}

template<>
void World::OnComponentAdded<Tag>(Entity Entity, Tag& Component)
{
}

template<>
void World::OnComponentAdded<Transform>(Entity Entity, Transform& Component)
{
}

template<>
void World::OnComponentAdded<Camera>(Entity Entity, Camera& Component)
{
	if (!ActiveCamera)
	{
		ActiveCamera = &Component;
		Entity.AddComponent<NativeScript>().Bind<PlayerScript>();
	}
	else
	{
		Component.Main = false;
	}
}

template<>
void World::OnComponentAdded<Light>(Entity Entity, Light& Component)
{
}

template<>
void World::OnComponentAdded<MeshFilter>(Entity Entity, MeshFilter& Component)
{
	// If we added a MeshFilter component, check to see if we have a MeshRenderer component
	// and connect them
	if (Entity.HasComponent<MeshRenderer>())
	{
		MeshRenderer& MeshRendererComponent = Entity.GetComponent<MeshRenderer>();
		MeshRendererComponent.pMeshFilter	= &Component;

		WorldState |= EWorldState_Update;
	}

	if (Entity.HasComponent<MeshCollider>())
	{
		MeshCollider& MeshColliderComponent = Entity.GetComponent<MeshCollider>();

		if (Component.Mesh)
		{
			MeshColliderComponent.Vertices = Component.Mesh->Vertices;
			MeshColliderComponent.Indices  = Component.Mesh->Indices;
		}
	}
}

template<>
void World::OnComponentAdded<MeshRenderer>(Entity Entity, MeshRenderer& Component)
{
	// If we added a MeshRenderer component, check to see if we have a MeshFilter component
	// and connect them
	if (Entity.HasComponent<MeshFilter>())
	{
		MeshFilter& MeshFilterComponent = Entity.GetComponent<MeshFilter>();
		Component.pMeshFilter			= &MeshFilterComponent;

		WorldState |= EWorldState_Update;
	}
}

template<>
void World::OnComponentAdded<CharacterController>(Entity Entity, CharacterController& Component)
{
	Component.Controller->setPosition(ToPxExtendedVec3(Entity.GetComponent<Transform>().Position));
}

template<>
void World::OnComponentAdded<NativeScript>(Entity Entity, NativeScript& Component)
{
}

template<>
void World::OnComponentAdded<BoxCollider>(Entity Entity, BoxCollider& Component)
{
}

template<>
void World::OnComponentAdded<CapsuleCollider>(Entity Entity, CapsuleCollider& Component)
{
}

template<>
void World::OnComponentAdded<MeshCollider>(Entity Entity, MeshCollider& Component)
{
	if (Entity.HasComponent<MeshFilter>())
	{
		MeshFilter& MeshFilterComponent = Entity.GetComponent<MeshFilter>();
		if (MeshFilterComponent.Mesh)
		{
			Component.Vertices = MeshFilterComponent.Mesh->Vertices;
			Component.Indices  = MeshFilterComponent.Mesh->Indices;
		}
	}
}

template<>
void World::OnComponentAdded<StaticRigidBody>(Entity Entity, StaticRigidBody& Component)
{
	if (Entity.HasComponent<BoxCollider>())
	{
		Component.Actor = PhysicsManager::AddStaticActorEntity(Entity, Entity.GetComponent<BoxCollider>());
	}
	else if (Entity.HasComponent<CapsuleCollider>())
	{
		Component.Actor = PhysicsManager::AddStaticActorEntity(Entity, Entity.GetComponent<CapsuleCollider>());
	}
	else if (Entity.HasComponent<MeshCollider>())
	{
		Component.Actor = PhysicsManager::AddGenericActor(Entity, Entity.GetComponent<MeshCollider>());
	}
}

template<>
void World::OnComponentAdded<DynamicRigidBody>(Entity Entity, DynamicRigidBody& Component)
{
	if (Entity.HasComponent<BoxCollider>())
	{
		Component.Actor = PhysicsManager::AddDynamicActorEntity(Entity, Entity.GetComponent<BoxCollider>());
	}
	else if (Entity.HasComponent<CapsuleCollider>())
	{
		Component.Actor = PhysicsManager::AddDynamicActorEntity(Entity, Entity.GetComponent<CapsuleCollider>());
	}
}

//===============================================================================================================================
template<typename T>
void World::OnComponentRemoved(Entity Entity, T& Component)
{
}

template<>
void World::OnComponentRemoved<Tag>(Entity Entity, Tag& Component)
{
}

template<>
void World::OnComponentRemoved<Transform>(Entity Entity, Transform& Component)
{
}

template<>
void World::OnComponentRemoved<Camera>(Entity Entity, Camera& Component)
{
}

template<>
void World::OnComponentRemoved<Light>(Entity Entity, Light& Component)
{
}

template<>
void World::OnComponentRemoved<MeshFilter>(Entity Entity, MeshFilter& Component)
{
}

template<>
void World::OnComponentRemoved<MeshRenderer>(Entity Entity, MeshRenderer& Component)
{
}

template<>
void World::OnComponentRemoved<CharacterController>(Entity Entity, CharacterController& Component)
{
}

template<>
void World::OnComponentRemoved<NativeScript>(Entity Entity, NativeScript& Component)
{
}

template<>
void World::OnComponentRemoved<BoxCollider>(Entity Entity, BoxCollider& Component)
{
}

template<>
void World::OnComponentRemoved<CapsuleCollider>(Entity Entity, CapsuleCollider& Component)
{
}

template<>
void World::OnComponentRemoved<MeshCollider>(Entity Entity, MeshCollider& Component)
{
}

template<>
void World::OnComponentRemoved<StaticRigidBody>(Entity Entity, StaticRigidBody& Component)
{
}

template<>
void World::OnComponentRemoved<DynamicRigidBody>(Entity Entity, DynamicRigidBody& Component)
{
}
