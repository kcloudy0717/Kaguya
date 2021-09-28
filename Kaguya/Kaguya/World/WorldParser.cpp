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

void to_json(json& Json, const MaterialTexture& InMaterialTexture)
{
	Json = InMaterialTexture.Path;
}

void from_json(const json& Json, MaterialTexture& OutMaterialTexture)
{
	OutMaterialTexture.Path = Json.get<std::string>();
}

void to_json(json& Json, const Material& InMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			Json[Attribute.GetName()] = Attribute.Get(InMaterial);
		});
}

void from_json(const json& json, Material& OutMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			Attribute.Set(OutMaterial, json[Attribute.GetName()].template get<decltype(Attribute.GetType())>());
		});
}

static void SerializeImages(json& Json)
{
	auto& JsonImages = Json["Images"];

	std::map<std::string, AssetHandle<Asset::Image>> SortedImages;
	AssetManager::GetImageCache().Each(
		[&](UINT64 Key, AssetHandle<Asset::Image> Resource)
		{
			SortedImages.insert({ Resource->Metadata.Path.string(), Resource });
		});

	for (auto& Handle : SortedImages | std::views::values)
	{
		auto& Image =
			JsonImages[std::filesystem::relative(Handle->Metadata.Path, Application::ExecutableDirectory).string()];
		Image["Metadata"]["SRGB"] = Handle->Metadata.sRGB;
	}
}

static void SerializeMeshes(json& Json)
{
	auto& JsonMeshes = Json["Meshes"];

	std::map<std::string, AssetHandle<Asset::Mesh>> SortedMeshes;
	AssetManager::GetMeshCache().Each(
		[&](UINT64 Key, AssetHandle<Asset::Mesh> Resource)
		{
			SortedMeshes.insert({ Resource->Metadata.Path.string(), Resource });
		});

	for (auto& Handle : SortedMeshes | std::views::values)
	{
		auto& Mesh =
			JsonMeshes[std::filesystem::relative(Handle->Metadata.Path, Application::ExecutableDirectory).string()];
		Mesh["Metadata"]["KeepGeometryInRAM"] = Handle->Metadata.KeepGeometryInRAM;
	}
}

template<is_component T>
struct ComponentSerializer
{
	ComponentSerializer(json& Json, Entity Entity)
	{
		if (Entity.HasComponent<T>())
		{
			auto& Component = Entity.GetComponent<T>();

			auto& JsonAttributes = Json[GetClass<T>()];
			ForEachAttribute<T>(
				[&](auto&& Attribute)
				{
					JsonAttributes[Attribute.GetName()] = Attribute.Get(Component);
				});
		}
	}
};

void WorldParser::Save(const std::filesystem::path& Path, World* World)
{
	json Json;
	{
		Json["Version"] = Version::String;

		SerializeImages(Json);
		SerializeMeshes(Json);

		auto& JsonWorld = Json["World"];
		UINT  EntityUID = 0;
		World->Registry.each(
			[&](auto Handle)
			{
				Entity Entity(Handle, World);

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
	ofs << std::setw(4) << Json << std::endl;
}

template<is_component T>
struct ComponentDeserializer
{
	ComponentDeserializer(const json& Json, Entity* Entity)
	{
		if (auto iter = Json.find(GetClass<T>()); iter != Json.end())
		{
			auto& Component = Entity->GetOrAddComponent<T>();

			ForEachAttribute<T>(
				[&](auto&& Attribute)
				{
					Attribute.Set(
						Component,
						iter.value()[Attribute.GetName()].template get<decltype(Attribute.GetType())>());
				});
		}
	}
};

void WorldParser::Load(const std::filesystem::path& Path, World* World)
{
	World->Clear(false);

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
		std::string ImagePath	 = (Application::ExecutableDirectory / it.key()).string();
		auto&		JsonMetadata = it.value()["Metadata"];
		bool		sRGB		 = JsonMetadata["SRGB"].get<bool>();

		AssetManager::AsyncLoadImage(ImagePath, sRGB);
	}

	const auto& JsonMeshes = json["Meshes"];
	for (auto it = JsonMeshes.begin(); it != JsonMeshes.end(); ++it)
	{
		std::string MeshPath		  = (Application::ExecutableDirectory / it.key()).string();
		auto&		JsonMetadata	  = it.value()["Metadata"];
		bool		KeepGeometryInRAM = JsonMetadata["KeepGeometryInRAM"].get<bool>();

		AssetManager::AsyncLoadMesh(MeshPath, KeepGeometryInRAM);
	}

	for (const auto& JsonEntity : json["World"])
	{
		Entity Entity = World->CreateEntity();

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
