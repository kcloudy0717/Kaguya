#pragma once
#include <entt.hpp>

class World;

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
	auto AddComponent(TArgs&&... Args) -> T&;

	template<typename T>
	[[nodiscard]] auto GetComponent() -> T&;

	template<typename T, typename... TArgs>
	[[nodiscard]] auto GetOrAddComponent(TArgs&&... Args) -> T&;

	template<typename T>
	[[nodiscard]] auto HasComponent() -> bool;

	template<typename T>
	void RemoveComponent();

	void OnComponentModified();

	operator bool() const noexcept;

	operator entt::entity() const noexcept { return Handle; }

	auto operator<=>(const Entity&) const = default;

	Entity Clone();

private:
	entt::entity Handle = entt::null;
	World*		 World	= nullptr;
};

class ScriptableEntity
{
public:
	virtual ~ScriptableEntity() = default;

	template<typename T>
	[[nodiscard]] T& GetComponent()
	{
		return Entity.GetComponent<T>();
	}

	template<typename T>
	[[nodiscard]] bool HasComponent() const
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
