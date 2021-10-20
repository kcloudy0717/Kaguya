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

template<typename T>
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

		auto& JsonTextures = Json["Textures"];
		AssetManager::GetTextureCache().Each(
			[&](AssetHandle Handle, Texture* Resource)
			{
				std::filesystem::path AssetPath	  = relative(Resource->Options.Path, Application::ExecutableDirectory);
				auto&				  JsonTexture = JsonTextures[AssetPath.string()];
				JsonTexture["Options"]["sRGB"]	  = Resource->Options.sRGB;
				JsonTexture["Options"]["GenerateMips"] = Resource->Options.GenerateMips;
			});

		auto& JsonMeshes = Json["Meshes"];
		AssetManager::GetMeshCache().Each(
			[&](AssetHandle Handle, Mesh* Resource)
			{
				std::filesystem::path AssetPath = relative(Resource->Options.Path, Application::ExecutableDirectory);
				auto&				  Mesh		= JsonMeshes[AssetPath.string()];
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

template<typename T>
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

template<typename T>
void JsonGetIfExists(const json::value_type& Json, const char* Key, T& RefValue)
{
	if (Json.contains(Key))
	{
		RefValue = Json[Key].get<T>();
	}
}

void WorldParser::Load(const std::filesystem::path& Path, World* World)
{
	std::ifstream ifs(Path);
	json		  Json;
	ifs >> Json;

	if (Json.contains("Version"))
	{
		if (Version::String != Json["Version"].get<std::string>())
		{
			throw std::exception("Invalid version");
		}
	}

	if (Json.contains("Textures"))
	{
		AssetManager::GetTextureCache().DestroyAll();

		const auto& JsonTextures = Json["Textures"];
		for (auto iter = JsonTextures.begin(); iter != JsonTextures.end(); ++iter)
		{
			TextureImportOptions Options = {};
			Options.Path				 = Application::ExecutableDirectory / iter.key();

			if (auto& Value = iter.value(); Value.contains("Options"))
			{
				auto& JsonOptions = Value["Options"];
				JsonGetIfExists<bool>(JsonOptions, "sRGB", Options.sRGB);
				JsonGetIfExists<bool>(JsonOptions, "GenerateMips", Options.GenerateMips);
			}

			AssetManager::AsyncLoadImage(Options);
		}
	}

	if (Json.contains("Meshes"))
	{
		AssetManager::GetMeshCache().DestroyAll();

		const auto& JsonMeshes = Json["Meshes"];
		for (auto iter = JsonMeshes.begin(); iter != JsonMeshes.end(); ++iter)
		{
			MeshImportOptions Options = {};
			Options.Path			  = Application::ExecutableDirectory / iter.key();

			auto& Value = iter.value();
			if (Value.contains("Options"))
			{
				auto& JsonOptions = Value["Options"];
			}

			AssetManager::AsyncLoadMesh(Options);
		}
	}

	if (Json.contains("World"))
	{
		World->Clear(false);

		const auto& JsonWorld = Json["World"];
		for (const auto& JsonEntity : JsonWorld)
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
}
