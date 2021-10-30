#pragma once
#include <filesystem>

class World;

class WorldArchive
{
public:
	static void Save(const std::filesystem::path& Path, World* World);

	static void Load(const std::filesystem::path& Path, World* World);
};
