#pragma once

struct Component
{
};

template<class T>
concept is_component = std::is_base_of_v<Component, T>;
