#include "WorldParser.h"
#include "World.h"
#include "Entity.h"
#include <Core/Asset/AssetManager.h>

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
	ForEachAttribute<MaterialTexture>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			Json[Name]		 = Attribute.Get(InMaterialTexture);
		});
}
void from_json(const json& Json, MaterialTexture& OutMaterialTexture)
{
	ForEachAttribute<MaterialTexture>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			if (Json.contains(Name))
			{
				Attribute.Set(OutMaterialTexture, Json[Name].get<decltype(Attribute.GetType())>());
			}
		});

	OutMaterialTexture.Handle.Type	= AssetType::Texture;
	OutMaterialTexture.Handle.State = AssetState::Dirty;
	OutMaterialTexture.Handle.Id	= OutMaterialTexture.HandleId;
}

void to_json(json& Json, const Material& InMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			Json[Name]		 = Attribute.Get(InMaterial);
		});
}
void from_json(const json& Json, Material& OutMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			if (Json.contains(Name))
			{
				Attribute.Set(OutMaterial, Json[Name].get<decltype(Attribute.GetType())>());
			}
		});
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
					const char* Name	 = Attribute.GetName();
					JsonAttributes[Name] = Attribute.Get(Component);
				});
		}
	}
};

void WorldParser::Save(const std::filesystem::path& Path, World* World)
{
	json Json;
	{
		Json["Version"] = Version::String;

		auto& JsonImages = Json["Images"];
		AssetManager::GetTextureCache().Each(
			[&](AssetHandle Handle, Texture* Resource)
			{
				auto& Image = JsonImages[relative(Resource->Metadata.Path, Application::ExecutableDirectory).string()];
				Image["Metadata"]["SRGB"] = Resource->Metadata.sRGB;
			});

		auto& JsonMeshes = Json["Meshes"];

		AssetManager::GetMeshCache().Each(
			[&](AssetHandle Handle, Mesh* Resource)
			{
				auto& Mesh = JsonMeshes[relative(Resource->Metadata.Path, Application::ExecutableDirectory).string()];
				Mesh["Metadata"]["KeepGeometryInRAM"] = Resource->Metadata.KeepGeometryInRAM;
			});

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
					const auto& Value = iter.value();

					const char* Name = Attribute.GetName();
					if (Value.contains(Name))
					{
						Attribute.Set(Component, Value[Name].template get<decltype(Attribute.GetType())>());
					}
				});
		}
	}
};

void WorldParser::Load(const std::filesystem::path& Path, World* World)
{
	World->Clear(false);

	AssetManager::GetTextureCache().DestroyAll();
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
			auto& MeshFilterComponent		 = Entity.GetComponent<MeshFilter>();
			MeshFilterComponent.Handle.Type	 = AssetType::Mesh;
			MeshFilterComponent.Handle.State = AssetState::Dirty;
			MeshFilterComponent.Handle.Id	 = MeshFilterComponent.HandleId;
		}
	}
}
