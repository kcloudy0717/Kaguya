#include "World.h"
#include "Actor.h"
#include "Core/Asset/AssetManager.h"

static const char* DefaultActorName = "Actor";

World::World(Asset::AssetManager* AssetManager)
	: AssetManager(AssetManager)
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

void World::Clear(bool AddDefaultEntities /*= true*/)
{
	WorldState = EWorldState_Update;
	Registry.clear();
	ActiveCameraActor	= {};
	ActiveSkyLightActor = {};
	ActiveCamera		= nullptr;
	ActiveSkyLight		= nullptr;
	Actors.clear();
	if (AddDefaultEntities)
	{
		ActiveCameraActor = CreateActor("Main Camera");
		ActiveCameraActor.AddComponent<CameraComponent>();

		ActiveSkyLightActor = CreateActor("Main Sky Light");
		ActiveSkyLightActor.AddComponent<SkyLightComponent>();
	}
}

void World::DestroyActor(size_t Index)
{
	Actor Entity = Actors[Index];
	if (Entity == ActiveCameraActor || Entity == ActiveSkyLightActor)
	{
		return;
	}
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

void World::ResolveComponentDependencies()
{
	// Refresh StaticMeshComponent
	Registry.view<StaticMeshComponent>().each(
		[this](StaticMeshComponent& StaticMesh)
		{
			{
				auto Handle			= StaticMesh.Handle;
				StaticMesh.Mesh		= AssetManager->GetMeshRegistry().GetValidAsset(Handle);
				StaticMesh.HandleId = Handle.Id;
			}

			{
				auto Handle	 = StaticMesh.Material.Albedo.Handle;
				auto Texture = AssetManager->GetTextureRegistry().GetValidAsset(Handle);
				if (Texture)
				{
					StaticMesh.Material.Albedo.HandleId	  = Handle.Id;
					StaticMesh.Material.TextureIndices[0] = Texture->Srv.GetIndex();
				}
			}
		});

	Registry.view<SkyLightComponent>().each(
		[this](SkyLightComponent& SkyLight)
		{
			auto Handle		 = SkyLight.Handle;
			auto Texture	 = AssetManager->GetTextureRegistry().GetValidAsset(Handle);
			SkyLight.Texture = Texture;
			if (Texture)
			{
				SkyLight.HandleId = Handle.Id;
				SkyLight.SRVIndex = Texture->Srv.GetIndex();
			}
			else
			{
				SkyLight.SRVIndex = -1;
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
		ActiveCameraActor = Actor;
		ActiveCamera	  = &Component;
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
void World::OnComponentAdded<SkyLightComponent>(Actor Actor, SkyLightComponent& Component)
{
	if (!ActiveSkyLight)
	{
		ActiveSkyLightActor = Actor;
		ActiveSkyLight		= &Component;
	}
	else
	{
		Component.Main = false;
	}
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
void World::OnComponentRemoved<SkyLightComponent>(Actor Actor, SkyLightComponent& Component)
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
