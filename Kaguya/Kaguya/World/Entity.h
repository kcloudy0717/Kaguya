#pragma once
#include <entt.hpp>
#include "World.h"

class Entity
{
public:
	Entity() noexcept = default;
	Entity(entt::entity Handle, World* pWorld) noexcept
		: Handle(Handle)
		, pWorld(pWorld)
	{
	}

	template<typename T, typename... TArgs>
	auto AddComponent(TArgs&&... Args) -> T&
	{
		assert(!HasComponent<T>());

		T& Component = pWorld->Registry.emplace<T>(Handle, std::forward<TArgs>(Args)...);
		pWorld->OnComponentAdded<T>(*this, Component);
		return Component;
	}

	template<typename T>
	[[nodiscard]] auto GetComponent() -> T&
	{
		assert(HasComponent<T>());

		return pWorld->Registry.get<T>(Handle);
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
		return pWorld->Registry.any_of<T>(Handle);
	}

	template<typename T>
	void RemoveComponent()
	{
		assert(HasComponent<T>());

		T& Component = GetComponent<T>();
		pWorld->OnComponentRemoved<T>(*this, Component);

		pWorld->Registry.remove<T>(Handle);
	}

	void OnComponentModified() { pWorld->WorldState |= EWorldState_Update; }

	operator bool() const noexcept { return Handle != entt::null; }

	operator entt::entity() const noexcept { return Handle; }

	auto operator<=>(const Entity&) const = default;

private:
	entt::entity Handle = entt::null;
	World*		 pWorld = nullptr;
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
	virtual void OnUpdate(float dt) {}

private:
	friend class World;

	Entity Entity;
};
