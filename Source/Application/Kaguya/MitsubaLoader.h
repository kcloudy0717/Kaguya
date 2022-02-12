#pragma once
#include <System.h>
#include <Core/Asset/AssetManager.h>
#include <Core/World/World.h>

class MitsubaLoader
{
public:
	static void Load(const std::filesystem::path& Path, Asset::AssetManager* AssetManager, World* World);

private:
	static void ParseNode(const System::Xml::XmlNode* Node, Asset::AssetManager* AssetManager, World* World);
};
