#pragma once
#include <filesystem>

class World;

namespace Asset
{
	class AssetManager;
}

class WorldArchive
{
public:
	static void Save(
		const std::filesystem::path& Path,
		World*						 World,
		Asset::AssetManager*		 AssetManager);

	static void Load(
		const std::filesystem::path& Path,
		World*						 World,
		Asset::AssetManager*		 AssetManager);
};
