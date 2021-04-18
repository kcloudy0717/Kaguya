#pragma once
#include <filesystem>

struct Scene;

class SceneParser
{
public:
	static void Save(const std::filesystem::path& Path, Scene* pScene);

	static void Load(const std::filesystem::path& Path, Scene* pScene);
};