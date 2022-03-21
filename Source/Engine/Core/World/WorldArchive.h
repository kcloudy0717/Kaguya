#pragma once
#include <filesystem>
#include "World.h"

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
		CameraComponent*			 Camera,
		Asset::AssetManager*		 AssetManager);

	static void Load(
		const std::filesystem::path& Path,
		World*						 World,
		CameraComponent*			 Camera,
		Asset::AssetManager*		 AssetManager);
};
