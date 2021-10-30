#include "World.h"
#include "Entity.h"
#include "Physics/PhysicsDevice.h"
#include "Core/Asset/AssetManager.h"

#include "Scripts/Player.script.h"

static const char* DefaultEntityName = "GameObject";

World::World()
{
	Clear(true);
}

void World::Initialize(PhysicsDevice* Device)
{
	PhysicsScene = Device->CreateScene();
}

auto World::CreateEntity(std::string_view Name /*= {}*/) -> Entity
{
	Entity Entity = Entities.emplace_back(Registry.create(), this);
	auto&  Core	  = Entity.AddComponent<CoreComponent>();
	Core.Name	  = Name.empty() ? DefaultEntityName : Name;
	return Entity;
}

auto World::GetMainCamera() -> Entity
{
	auto View = Registry.view<CameraComponent>();
	for (entt::entity Handle : View)
	{
		const auto& Component = View.get<CameraComponent>(Handle);
		if (Component.Main)
		{
			return Entity(Handle, this);
		}
	}
	return Entity(entt::null, this);
}

void World::Clear(bool AddDefaultEntities /*= true*/)
{
	WorldState = EWorldState_Update;
	Registry.clear();
	ActiveCamera = nullptr;
	Entities.clear();
	PhysicsScene.Reset();
	if (AddDefaultEntities)
	{
		Entity MainCamera = CreateEntity("Main Camera");
		MainCamera.AddComponent<CameraComponent>();
	}
}

void World::DestroyEntity(size_t Index)
{
	Entity Entity = Entities[Index];
	Registry.destroy(Entity);
	Entities.erase(Entities.begin() + Index);
}

void World::CloneEntity(size_t Index)
{
	Entity Entity = Entities[Index];
	Entity.Clone();
}

void World::Update(float DeltaTime)
{
	ResolveComponentDependencies();
	UpdateScripts(DeltaTime);
	if (SimulatePhysics)
	{
		PhysicsScene.Simulate(DeltaTime);
	}
}

void World::BeginPlay()
{
	PhysicsScene.Reset();

	for (auto Entity : Entities)
	{
		if (Entity.HasComponent<StaticRigidBodyComponent>())
		{
			if (Entity.HasComponent<BoxColliderComponent>())
			{
				PhysicsScene.AddStaticActor(Entity, Entity.GetComponent<BoxColliderComponent>());
			}
			else if (Entity.HasComponent<CapsuleColliderComponent>())
			{
				PhysicsScene.AddStaticActor(Entity, Entity.GetComponent<CapsuleColliderComponent>());
			}
		}
		else if (Entity.HasComponent<DynamicRigidBodyComponent>())
		{
			if (Entity.HasComponent<BoxColliderComponent>())
			{
				PhysicsScene.AddDynamicActor(Entity, Entity.GetComponent<BoxColliderComponent>());
			}
			else if (Entity.HasComponent<CapsuleColliderComponent>())
			{
				PhysicsScene.AddDynamicActor(Entity, Entity.GetComponent<CapsuleColliderComponent>());
			}
		}
	}
}

void World::ResolveComponentDependencies()
{
	Registry.view<CoreComponent, CameraComponent>().each(
		[&](CoreComponent& Core, CameraComponent& Camera)
		{
			Camera.pTransform = &Core.Transform;

			if (Camera.Dirty)
			{
				Camera.Dirty = false;
				WorldState |= EWorldState_Update;
			}
		});

	// Refresh StaticMeshComponent
	Registry.view<StaticMeshComponent>().each(
		[](StaticMeshComponent& StaticMesh)
		{
			{
				auto Handle			= StaticMesh.Handle;
				StaticMesh.Mesh		= AssetManager::GetMeshCache().GetValidAsset(Handle);
				StaticMesh.HandleId = Handle.Id;
			}

			{
				auto Handle	 = StaticMesh.Material.Albedo.Handle;
				auto Texture = AssetManager::GetTextureCache().GetValidAsset(Handle);
				if (Texture)
				{
					StaticMesh.Material.Albedo.HandleId	  = Handle.Id;
					StaticMesh.Material.TextureIndices[0] = Texture->SRV.GetIndex();
				}
			}
		});
}

void World::UpdateScripts(float DeltaTime)
{
	Registry.view<NativeScriptComponent>().each(
		[&](auto Handle, NativeScriptComponent& NativeScript)
		{
			if (!NativeScript.Instance)
			{
				NativeScript.Instance		  = NativeScript.InstantiateScript();
				NativeScript.Instance->Entity = Entity{ Handle, this };
				NativeScript.Instance->OnCreate();
			}

			NativeScript.Instance->OnUpdate(DeltaTime);
		});
}

template<typename T>
void World::OnComponentAdded(Entity Entity, T& Component)
{
}

template<>
void World::OnComponentAdded<CoreComponent>(Entity Entity, CoreComponent& Component)
{
}

template<>
void World::OnComponentAdded<CameraComponent>(Entity Entity, CameraComponent& Component)
{
	if (!ActiveCamera)
	{
		ActiveCamera = &Component;
		Entity.AddComponent<NativeScriptComponent>().Bind<PlayerScript>();
	}
	else
	{
		Component.Main = false;
	}
}

template<>
void World::OnComponentAdded<LightComponent>(Entity Entity, LightComponent& Component)
{
}

template<>
void World::OnComponentAdded<StaticMeshComponent>(Entity Entity, StaticMeshComponent& Component)
{
	if (Entity.HasComponent<MeshColliderComponent>())
	{
		auto& MeshCollider = Entity.GetComponent<MeshColliderComponent>();
		if (Component.Mesh)
		{
			MeshCollider.Vertices = Component.Mesh->Vertices;
			MeshCollider.Indices  = Component.Mesh->Indices;
		}
	}
}

template<>
void World::OnComponentAdded<CharacterControllerComponent>(Entity Entity, CharacterControllerComponent& Component)
{
	Component.Controller->setPosition(ToPxExtendedVec3(Entity.GetComponent<CoreComponent>().Transform.Position));
}

template<>
void World::OnComponentAdded<NativeScriptComponent>(Entity Entity, NativeScriptComponent& Component)
{
}

template<>
void World::OnComponentAdded<BoxColliderComponent>(Entity Entity, BoxColliderComponent& Component)
{
}

template<>
void World::OnComponentAdded<CapsuleColliderComponent>(Entity Entity, CapsuleColliderComponent& Component)
{
}

template<>
void World::OnComponentAdded<MeshColliderComponent>(Entity Entity, MeshColliderComponent& Component)
{
	if (Entity.HasComponent<StaticMeshComponent>())
	{
		auto& StaticMesh = Entity.GetComponent<StaticMeshComponent>();
		if (StaticMesh.Mesh)
		{
			Component.Vertices = StaticMesh.Mesh->Vertices;
			Component.Indices  = StaticMesh.Mesh->Indices;
		}
	}
}

template<>
void World::OnComponentAdded<StaticRigidBodyComponent>(Entity Entity, StaticRigidBodyComponent& Component)
{
}

template<>
void World::OnComponentAdded<DynamicRigidBodyComponent>(Entity Entity, DynamicRigidBodyComponent& Component)
{
}

//===============================================================================================================================
template<typename T>
void World::OnComponentRemoved(Entity Entity, T& Component)
{
}

template<>
void World::OnComponentRemoved<CoreComponent>(Entity Entity, CoreComponent& Component)
{
}

template<>
void World::OnComponentRemoved<CameraComponent>(Entity Entity, CameraComponent& Component)
{
}

template<>
void World::OnComponentRemoved<LightComponent>(Entity Entity, LightComponent& Component)
{
}

template<>
void World::OnComponentRemoved<StaticMeshComponent>(Entity Entity, StaticMeshComponent& Component)
{
}

template<>
void World::OnComponentRemoved<CharacterControllerComponent>(Entity Entity, CharacterControllerComponent& Component)
{
}

template<>
void World::OnComponentRemoved<NativeScriptComponent>(Entity Entity, NativeScriptComponent& Component)
{
}

template<>
void World::OnComponentRemoved<BoxColliderComponent>(Entity Entity, BoxColliderComponent& Component)
{
}

template<>
void World::OnComponentRemoved<CapsuleColliderComponent>(Entity Entity, CapsuleColliderComponent& Component)
{
}

template<>
void World::OnComponentRemoved<MeshColliderComponent>(Entity Entity, MeshColliderComponent& Component)
{
}

template<>
void World::OnComponentRemoved<StaticRigidBodyComponent>(Entity Entity, StaticRigidBodyComponent& Component)
{
}

template<>
void World::OnComponentRemoved<DynamicRigidBodyComponent>(Entity Entity, DynamicRigidBodyComponent& Component)
{
}
