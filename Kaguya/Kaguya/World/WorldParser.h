#pragma once
#include <filesystem>

class World;

class WorldParser
{
public:
	static void Save(const std::filesystem::path& Path, World* pWorld);

	static void Load(const std::filesystem::path& Path, World* pWorld);
};
