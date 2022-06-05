#pragma once
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <entt.hpp>
#include "Components.h"
#include "Actor.h"
#include "Math/Math.h"
#include "RHI/RHI.h"

namespace Asset
{
	class AssetManager;
}

enum EWorldState
{
	EWorldState_Render,
	EWorldState_Update
};
DEFINE_ENUM_FLAG_OPERATORS(EWorldState);

class World
{
public:
	static constexpr size_t MaterialLimit = 8192;
	static constexpr size_t LightLimit	  = 32;
	static constexpr size_t MeshLimit	  = 8192;

	World(Asset::AssetManager* AssetManager);

	[[nodiscard]] auto CreateActor(std::string_view Name = {}) -> Actor;

	void Clear(bool AddDefaultEntities = true);

	void DestroyActor(size_t Index);
	void CloneActor(size_t Index);

	template<typename T>
	void OnComponentAdded(Actor Actor, T& Component);

	template<typename T>
	void OnComponentRemoved(Actor Actor, T& Component);

	void Update(float DeltaTime);

private:
	void ResolveComponentDependencies();
	void UpdateScripts(float DeltaTime);

public:
	Asset::AssetManager* AssetManager = nullptr;

	EWorldState	   WorldState = EWorldState::EWorldState_Render;
	entt::registry Registry;

	Actor			   ActiveSkyLightActor;
	SkyLightComponent* ActiveSkyLight = nullptr;
	std::vector<Actor> Actors;
};

template<typename T, typename... TArgs>
auto Actor::AddComponent(TArgs&&... Args) -> T&
{
	assert(!HasComponent<T>());

	T& Component = World->Registry.emplace<T>(Handle, std::forward<TArgs>(Args)...);
	World->OnComponentAdded<T>(*this, Component);
	return Component;
}

template<typename T>
[[nodiscard]] auto Actor::GetComponent() -> T&
{
	assert(HasComponent<T>());

	return World->Registry.get<T>(Handle);
}

template<typename T, typename... TArgs>
[[nodiscard]] auto Actor::GetOrAddComponent(TArgs&&... Args) -> T&
{
	if (HasComponent<T>())
	{
		return GetComponent<T>();
	}
	return AddComponent<T>(std::forward<TArgs>(Args)...);
}

template<typename T>
[[nodiscard]] auto Actor::HasComponent() -> bool
{
	return World->Registry.any_of<T>(Handle);
}

template<typename T>
void Actor::RemoveComponent()
{
	assert(HasComponent<T>());

	T& Component = GetComponent<T>();
	World->OnComponentRemoved<T>(*this, Component);

	World->Registry.remove<T>(Handle);
}
