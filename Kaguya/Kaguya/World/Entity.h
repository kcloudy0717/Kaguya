#pragma once
#include <entt.hpp>
#include "World.h"

class Entity
{
public:
	Entity() noexcept = default;
	Entity(entt::entity Handle, World* World) noexcept
		: Handle(Handle)
		, World(World)
	{
	}

	template<typename T, typename... TArgs>
	auto AddComponent(TArgs&&... Args) -> T&
	{
		assert(!HasComponent<T>());

		T& Component = World->Registry.emplace<T>(Handle, std::forward<TArgs>(Args)...);
		World->OnComponentAdded<T>(*this, Component);
		return Component;
	}

	template<typename T>
	[[nodiscard]] auto GetComponent() -> T&
	{
		assert(HasComponent<T>());

		return World->Registry.get<T>(Handle);
	}

	template<typename T, typename... TArgs>
	[[nodiscard]] auto GetOrAddComponent(TArgs&&... Args) -> T&
	{
		if (HasComponent<T>())
		{
			return GetComponent<T>();
		}
		return AddComponent<T>(std::forward<TArgs>(Args)...);
	}

	template<typename T>
	[[nodiscard]] auto HasComponent() -> bool
	{
		return World->Registry.any_of<T>(Handle);
	}

	template<typename T>
	void RemoveComponent()
	{
		assert(HasComponent<T>());

		T& Component = GetComponent<T>();
		World->OnComponentRemoved<T>(*this, Component);

		World->Registry.remove<T>(Handle);
	}

	void OnComponentModified() { World->WorldState |= EWorldState_Update; }

	operator bool() const noexcept { return Handle != entt::null; }

	operator entt::entity() const noexcept { return Handle; }

	auto operator<=>(const Entity&) const = default;

private:
	entt::entity Handle = entt::null;
	World*		 World	= nullptr;
};

class ScriptableEntity
{
public:
	virtual ~ScriptableEntity() = default;

	template<typename T>
	T& GetComponent()
	{
		return Entity.GetComponent<T>();
	}

	template<typename T>
	bool HasComponent() const
	{
		return Entity.HasComponent<T>();
	}

protected:
	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	virtual void OnUpdate(float DeltaTime) {}

private:
	friend class World;

	Entity Entity;
};
