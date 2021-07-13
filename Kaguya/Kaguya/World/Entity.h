#pragma once
#include <entt.hpp>
#include "World.h"

struct Entity
{
	Entity() noexcept = default;
	Entity(entt::entity Handle, World* pWorld)
		: Handle(Handle)
		, pWorld(pWorld)
	{
	}

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		assert(!HasComponent<T>());

		T& component = pWorld->Registry.emplace<T>(Handle, std::forward<Args>(args)...);
		pWorld->OnComponentAdded<T>(*this, component);
		return component;
	}

	template<typename T>
	T& GetComponent()
	{
		assert(HasComponent<T>());

		return pWorld->Registry.get<T>(Handle);
	}

	template<typename T, typename... Args>
	T& GetOrAddComponent(Args&&... args)
	{
		if (HasComponent<T>())
		{
			return GetComponent<T>();
		}
		return AddComponent<T>(args...);
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

		pWorld->Registry.remove<T>(Handle);
	}

	operator bool() const { return Handle != entt::null; }

	operator entt::entity() const { return Handle; }

	operator uint32_t() const { return (uint32_t)Handle; }

	bool operator==(const Entity& other) const { return Handle == other.Handle && pWorld == other.pWorld; }

	bool operator!=(const Entity& other) const { return !(*this == other); }

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
