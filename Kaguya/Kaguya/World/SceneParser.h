#pragma once
#include <filesystem>

class World;

class SceneParser
{
public:
	static void Save(const std::filesystem::path& Path, World* pWorld);

	static void Load(const std::filesystem::path& Path, World* pWorld);
};
