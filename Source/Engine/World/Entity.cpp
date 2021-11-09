#include "Entity.h"
#include "World.h"

void Entity::OnComponentModified()
{
	World->WorldState |= EWorldState_Update;
}

Entity::operator bool() const noexcept
{
	return Handle != entt::null && World->Registry.valid(Handle);
}

template<typename T>
static void CopyComponentIfExists(Entity Dst, Entity Src, entt::registry& Registry)
{
	if (Src.HasComponent<T>())
	{
		auto& Component = Src.GetComponent<T>();
		Registry.emplace_or_replace<T>(Dst, Component);
	}
}

Entity Entity::Clone()
{
	Entity Clone = World->CreateEntity();

	CopyComponentIfExists<CoreComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<CameraComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<LightComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<StaticMeshComponent>(Clone, *this, World->Registry);
	// CopyComponentIfExists<NativeScriptComponent>(Clone, *this, World->Registry);

	return Clone;
}
