#pragma once
#include <entt.hpp>
#include "World.h"

class Entity
{
public:
	Entity() noexcept = default;
	Entity(entt::entity Handle, World* pWorld)
		: Handle(Handle)
		, pWorld(pWorld)
	{
	}

	template<typename T, typename... TArgs>
	T& AddComponent(TArgs&&... Args)
	{
		assert(!HasComponent<T>());

		T& Component = pWorld->Registry.emplace<T>(Handle, std::forward<TArgs>(Args)...);
		pWorld->OnComponentAdded<T>(*this, Component);
		return Component;
	}

	template<typename T>
	T& GetComponent()
	{
		assert(HasComponent<T>());

		return pWorld->Registry.get<T>(Handle);
	}

	template<typename T, typename... TArgs>
	T& GetOrAddComponent(TArgs&&... Args)
	{
		if (HasComponent<T>())
		{
			return GetComponent<T>();
		}
		return AddComponent<T>(std::forward<TArgs>(Args)...);
	}

	template<typename T>
	bool HasComponent()
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

	bool operator==(const Entity& other) const noexcept { return Handle == other.Handle && pWorld == other.pWorld; }

	bool operator!=(const Entity& other) const noexcept { return !(*this == other); }

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
