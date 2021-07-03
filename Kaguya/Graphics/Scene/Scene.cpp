#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "../AssetManager.h"

Scene::Scene() noexcept
{
	Camera.Transform.Position = { 0.0f, 5.0f, -10.0f };
	PreviousCamera			  = Camera;

	CameraController = std::make_unique<FPSCamera>(Camera);
}

void Scene::Clear()
{
	SceneState = SceneState_Update;
	Registry.clear();
}

void Scene::Update(float dt)
{
	PreviousCamera = Camera;

	CameraController->Update(dt);

	// Update all mesh filters
	Registry.view<MeshFilter>().each(
		[](auto&& MeshFilter)
		{
			MeshFilter.Mesh = AssetManager::GetMeshCache().Load(MeshFilter.Key);
		});

	// Update all mesh renderers
	Registry.view<MeshFilter, MeshRenderer>().each(
		[](auto&& MeshFilter, auto&& MeshRenderer)
		{
			// I have to manually update the mesh filters here because I keep on getting
			// access violation when I add new mesh filter to entt, need to investigate
			MeshRenderer.pMeshFilter = &MeshFilter;

			for (int i = 0; i < TextureTypes::NumTextureTypes; ++i)
			{
				auto Texture = AssetManager::GetImageCache().Load(MeshRenderer.Material.TextureKeys[i]);

				if (Texture)
				{
					MeshRenderer.Material.Textures[i]		= Texture;
					MeshRenderer.Material.TextureIndices[i] = Texture->SRV.GetIndex();
				}
			}
		});

	if (PreviousCamera != Camera)
	{
		SceneState |= SceneState_Update;
	}
}

Entity Scene::CreateEntity(const std::string& Name)
{
	Entity NewEntity	= { Registry.create(), this };
	auto&  TagComponent = NewEntity.AddComponent<Tag>();
	TagComponent.Name	= Name;

	NewEntity.AddComponent<Transform>();

	return NewEntity;
}

void Scene::DestroyEntity(Entity Entity)
{
	Registry.destroy(Entity);
}

template<typename T>
void Scene::OnComponentAdded(Entity Entity, T& Component)
{
}

template<>
void Scene::OnComponentAdded<Tag>(Entity Entity, Tag& Component)
{
}

template<>
void Scene::OnComponentAdded<Transform>(Entity Entity, Transform& Component)
{
}

template<>
void Scene::OnComponentAdded<MeshFilter>(Entity Entity, MeshFilter& Component)
{
	// If we added a MeshFilter component, check to see if we have a MeshRenderer component
	// and connect them
	if (Entity.HasComponent<MeshRenderer>())
	{
		MeshRenderer& MeshRendererComponent = Entity.GetComponent<MeshRenderer>();
		MeshRendererComponent.pMeshFilter	= &Component;

		MeshRendererComponent.IsEdited = true;
	}
}

template<>
void Scene::OnComponentAdded<MeshRenderer>(Entity Entity, MeshRenderer& Component)
{
	// If we added a MeshRenderer component, check to see if we have a MeshFilter component
	// and connect them
	if (Entity.HasComponent<MeshFilter>())
	{
		MeshFilter& MeshFilterComponent = Entity.GetComponent<MeshFilter>();
		Component.pMeshFilter			= &MeshFilterComponent;

		Component.IsEdited = true;
	}
}

template<>
void Scene::OnComponentAdded<Light>(Entity Entity, Light& Component)
{
}
