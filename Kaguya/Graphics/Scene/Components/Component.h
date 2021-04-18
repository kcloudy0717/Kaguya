#pragma once
#include <entt.hpp>

struct Component
{
	entt::entity Handle = entt::null;
	struct Scene* pScene = nullptr;
	bool IsEdited = false;
};

template<class T>
concept IsAComponent = std::is_base_of_v<Component, T>;