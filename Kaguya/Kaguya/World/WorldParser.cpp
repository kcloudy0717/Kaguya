#include "WorldParser.h"
#include "World.h"
#include "Entity.h"
#include <Graphics/AssetManager.h>

#include <fstream>
#include <nlohmann/json.hpp>
// using json = nlohmann::json;
using json = nlohmann::ordered_json;

namespace Version
{
constexpr int	  Major	   = 1;
constexpr int	  Minor	   = 0;
constexpr int	  Revision = 0;
const std::string String   = std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision);
} // namespace Version

#define NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED(Type, ...)                                                          \
	inline void to_json(nlohmann::ordered_json& nlohmann_json_j, const Type& nlohmann_json_t)                          \
	{                                                                                                                  \
		NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                                       \
	}                                                                                                                  \
	inline void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t)                        \
	{                                                                                                                  \
		NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__))                                     \
	}

namespace DirectX
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED(XMFLOAT2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED(XMFLOAT3, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED(XMFLOAT4, x, y, z, w);
} // namespace DirectX

NLOHMANN_JSON_SERIALIZE_ENUM(
	ELightTypes,
	{
		{ ELightTypes::Point, "Point" },
		{ ELightTypes::Quad, "Quad" },
	});

NLOHMANN_JSON_SERIALIZE_ENUM(
	EBSDFTypes,
	{
		{ EBSDFTypes::Lambertian, "Lambertian" },
		{ EBSDFTypes::Mirror, "Mirror" },
		{ EBSDFTypes::Glass, "Glass" },
		{ EBSDFTypes::Disney, "Disney" },
	});

void to_json(json& json, const MaterialTexture& InMaterialTexture)
{
	json = InMaterialTexture.Path;
}

void from_json(const json& json, MaterialTexture& OutMaterialTexture)
{
	OutMaterialTexture.Path = json.get<std::string>();
}

void to_json(json& json, const Material& InMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			json[Attribute.GetName()] = Attribute.Get(InMaterial);
		});
}

void from_json(const json& json, Material& OutMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			Attribute.Set(OutMaterial, json[Attribute.GetName()].get<decltype(Attribute.GetType())>());
		});
}

static void SerializeImages(json& json)
{
	auto& ImageCache = AssetManager::GetImageCache();

	auto& Images = json["Images"];

	std::map<std::string, AssetHandle<Asset::Image>> SortedImages;
	ImageCache.Each_ThreadSafe(
		[&](UINT64 Key, AssetHandle<Asset::Image> Resource)
		{
			SortedImages.insert({ Resource->Metadata.Path.string(), Resource });
		});

	for (auto& SortedImage : SortedImages)
	{
		auto& Image =
			Images[std::filesystem::relative(SortedImage.second->Metadata.Path, Application::ExecutableDirectory)
					   .string()];
		Image["Metadata"]["SRGB"] = SortedImage.second->Metadata.sRGB;
	}
}

static void SerializeMeshes(json& json)
{
	auto& MeshCache = AssetManager::GetMeshCache();

	auto& Meshes = json["Meshes"];

	std::map<std::string, AssetHandle<Asset::Mesh>> SortedMeshes;
	MeshCache.Each_ThreadSafe(
		[&](UINT64 Key, AssetHandle<Asset::Mesh> Resource)
		{
			SortedMeshes.insert({ Resource->Metadata.Path.string(), Resource });
		});

	for (auto& SortedMeshe : SortedMeshes)
	{
		auto& Mesh =
			Meshes[std::filesystem::relative(SortedMeshe.second->Metadata.Path, Application::ExecutableDirectory)
					   .string()];
		Mesh["Metadata"]["KeepGeometryInRAM"] = SortedMeshe.second->Metadata.KeepGeometryInRAM;
	}
}

template<is_component T>
struct ComponentSerializer
{
	ComponentSerializer(json& json, Entity Entity)
	{
		if (Entity.HasComponent<T>())
		{
			auto& Component = Entity.GetComponent<T>();

			auto& JsonAttributes = json[GetClass<T>()];
			ForEachAttribute<T>(
				[&](auto&& Attribute)
				{
					JsonAttributes[Attribute.GetName()] = Attribute.Get(Component);
				});
		}
	}
};

void WorldParser::Save(const std::filesystem::path& Path, World* pWorld)
{
	json json;
	{
		json["Version"] = Version::String;

		SerializeImages(json);
		SerializeMeshes(json);

		auto& JsonWorld = json["World"];
		UINT  EntityUID = 0;
		pWorld->Registry.each(
			[&](auto Handle)
			{
				Entity Entity(Handle, pWorld);

				auto& JsonEntity = JsonWorld[EntityUID++];

				ComponentSerializer<Tag>(JsonEntity, Entity);
				ComponentSerializer<Transform>(JsonEntity, Entity);
				ComponentSerializer<Camera>(JsonEntity, Entity);
				ComponentSerializer<Light>(JsonEntity, Entity);
				ComponentSerializer<MeshFilter>(JsonEntity, Entity);
				ComponentSerializer<MeshRenderer>(JsonEntity, Entity);

				ComponentSerializer<BoxCollider>(JsonEntity, Entity);
				ComponentSerializer<CapsuleCollider>(JsonEntity, Entity);

				// ComponentSerializer<CharacterController>(Emitter, Entity);
				// ComponentSerializer<NativeScript>(Emitter, Entity);
				// ComponentSerializer<StaticRigidBody>(Emitter, Entity);
				// ComponentSerializer<DynamicRigidBody>(Emitter, Entity);
				//
				// ComponentSerializer<MeshCollider>(Emitter, Entity);
			});
	}

	std::ofstream ofs(Path);
	ofs << std::setw(4) << json << std::endl;
}

template<is_component T>
struct ComponentDeserializer
{
	ComponentDeserializer(const json& json, Entity* pEntity)
	{
		if (auto iter = json.find(GetClass<T>()); iter != json.end())
		{
			auto& Component = pEntity->GetOrAddComponent<T>();

			ForEachAttribute<T>(
				[&](auto&& Attribute)
				{
					Attribute.Set(Component, iter.value()[Attribute.GetName()].get<decltype(Attribute.GetType())>());
				});
		}
	}
};

void WorldParser::Load(const std::filesystem::path& Path, World* pWorld)
{
	pWorld->Clear(false);

	AssetManager::GetImageCache().DestroyAll();
	AssetManager::GetMeshCache().DestroyAll();

	std::ifstream ifs(Path);
	json		  json;
	ifs >> json;

	if (Version::String != json["Version"].get<std::string>())
	{
		throw std::exception("Invalid version");
	}

	const auto& JsonImages = json["Images"];
	for (auto it = JsonImages.begin(); it != JsonImages.end(); ++it)
	{
		std::string Path = (Application::ExecutableDirectory / it.key()).string();

		auto& JsonMetadata = it.value()["Metadata"];
		bool  sRGB		   = JsonMetadata["SRGB"].get<bool>();

		AssetManager::AsyncLoadImage(Path, sRGB);
	}

	const auto& JsonMeshes = json["Meshes"];
	for (auto it = JsonMeshes.begin(); it != JsonMeshes.end(); ++it)
	{
		std::string Path = (Application::ExecutableDirectory / it.key()).string();

		auto& JsonMetadata		= it.value()["Metadata"];
		bool  KeepGeometryInRAM = JsonMetadata["KeepGeometryInRAM"].get<bool>();

		AssetManager::AsyncLoadMesh(Path, KeepGeometryInRAM);
	}

	const auto& JsonWorld = json["World"];
	for (const auto& JsonEntity : JsonWorld)
	{
		Entity Entity = pWorld->CreateEntity();

		ComponentDeserializer<Tag>(JsonEntity, &Entity);
		ComponentDeserializer<Transform>(JsonEntity, &Entity);
		ComponentDeserializer<Camera>(JsonEntity, &Entity);
		ComponentDeserializer<Light>(JsonEntity, &Entity);
		ComponentDeserializer<MeshFilter>(JsonEntity, &Entity);
		ComponentDeserializer<MeshRenderer>(JsonEntity, &Entity);

		ComponentDeserializer<BoxCollider>(JsonEntity, &Entity);
		ComponentDeserializer<CapsuleCollider>(JsonEntity, &Entity);

		if (Entity.HasComponent<MeshFilter>())
		{
			auto& MeshFilterComponent = Entity.GetComponent<MeshFilter>();

			if (MeshFilterComponent.Path != "NULL")
			{
				MeshFilterComponent.Path = (Application::ExecutableDirectory / MeshFilterComponent.Path).string();

				MeshFilterComponent.Key = CityHash64(MeshFilterComponent.Path.data(), MeshFilterComponent.Path.size());
			}
		}

		if (Entity.HasComponent<MeshRenderer>())
		{
			auto&			 MeshRendererComponent = Entity.GetComponent<MeshRenderer>();
			MaterialTexture& Albedo				   = MeshRendererComponent.Material.Albedo;
			if (Albedo.Path != "NULL")
			{
				Albedo.Path = (Application::ExecutableDirectory / Albedo.Path).string();

				Albedo.Key = CityHash64(Albedo.Path.data(), Albedo.Path.size());
			}
		}
	}
}
