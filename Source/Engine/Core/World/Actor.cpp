#include "Actor.h"
#include "World.h"

void Actor::OnComponentModified()
{
	World->WorldState |= EWorldState_Update;
}

Actor::operator bool() const noexcept
{
	return Handle != entt::null && World->Registry.valid(Handle);
}

template<typename T>
static void CopyComponentIfExists(Actor Dst, Actor Src, entt::registry& Registry)
{
	if (Src.HasComponent<T>())
	{
		auto& Component = Src.GetComponent<T>();
		Registry.emplace_or_replace<T>(Dst, Component);
	}
}

Actor Actor::Clone()
{
	Actor Clone = World->CreateActor();

	CopyComponentIfExists<CoreComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<CameraComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<LightComponent>(Clone, *this, World->Registry);
	CopyComponentIfExists<StaticMeshComponent>(Clone, *this, World->Registry);
	// CopyComponentIfExists<NativeScriptComponent>(Clone, *this, World->Registry);

	return Clone;
}
