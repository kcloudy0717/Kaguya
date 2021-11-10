#include "World.h"
#include "Actor.h"
#include "Core/Asset/AssetManager.h"

#include "Scripts/Player.script.h"

static const char* DefaultActorName = "Actor";

World::World()
{
	Clear(true);
}

auto World::CreateActor(std::string_view Name /*= {}*/) -> Actor
{
	Actor Actor = Actors.emplace_back(Registry.create(), this);
	auto& Core	= Actor.AddComponent<CoreComponent>();
	Core.Name	= Name.empty() ? DefaultActorName : Name;
	return Actor;
}

auto World::GetMainCamera() -> Actor
{
	auto View = Registry.view<CameraComponent>();
	for (entt::entity Handle : View)
	{
		const auto& Component = View.get<CameraComponent>(Handle);
		if (Component.Main)
		{
			return Actor(Handle, this);
		}
	}
	return Actor(entt::null, this);
}

void World::Clear(bool AddDefaultEntities /*= true*/)
{
	WorldState = EWorldState_Update;
	Registry.clear();
	ActiveCamera = nullptr;
	Actors.clear();
	if (AddDefaultEntities)
	{
		Actor MainCamera = CreateActor("Main Camera");
		MainCamera.AddComponent<CameraComponent>();
	}
}

void World::DestroyActor(size_t Index)
{
	Actor Entity = Actors[Index];
	Registry.destroy(Entity);
	Actors.erase(Actors.begin() + Index);
}

void World::CloneActor(size_t Index)
{
	Actor Actor = Actors[Index];
	Actor.Clone();
}

void World::Update(float DeltaTime)
{
	ResolveComponentDependencies();
	UpdateScripts(DeltaTime);
}

void World::BeginPlay()
{
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
				NativeScript.Instance		 = NativeScript.InstantiateScript();
				NativeScript.Instance->Actor = Actor{ Handle, this };
				NativeScript.Instance->OnCreate();
			}

			NativeScript.Instance->OnUpdate(DeltaTime);
		});
}

template<typename T>
void World::OnComponentAdded(Actor Actor, T& Component)
{
}

template<>
void World::OnComponentAdded<CoreComponent>(Actor Actor, CoreComponent& Component)
{
}

template<>
void World::OnComponentAdded<CameraComponent>(Actor Actor, CameraComponent& Component)
{
	if (!ActiveCamera)
	{
		ActiveCamera = &Component;
		Actor.AddComponent<NativeScriptComponent>().Bind<PlayerScript>();
	}
	else
	{
		Component.Main = false;
	}
}

template<>
void World::OnComponentAdded<LightComponent>(Actor Actor, LightComponent& Component)
{
}

template<>
void World::OnComponentAdded<StaticMeshComponent>(Actor Actor, StaticMeshComponent& Component)
{
}

template<>
void World::OnComponentAdded<NativeScriptComponent>(Actor Actor, NativeScriptComponent& Component)
{
}

//===============================================================================================================================
template<typename T>
void World::OnComponentRemoved(Actor Actor, T& Component)
{
}

template<>
void World::OnComponentRemoved<CoreComponent>(Actor Actor, CoreComponent& Component)
{
}

template<>
void World::OnComponentRemoved<CameraComponent>(Actor Actor, CameraComponent& Component)
{
}

template<>
void World::OnComponentRemoved<LightComponent>(Actor Actor, LightComponent& Component)
{
}

template<>
void World::OnComponentRemoved<StaticMeshComponent>(Actor Actor, StaticMeshComponent& Component)
{
}

template<>
void World::OnComponentRemoved<NativeScriptComponent>(Actor Actor, NativeScriptComponent& Component)
{
}
