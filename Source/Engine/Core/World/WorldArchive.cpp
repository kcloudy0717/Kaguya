#include "WorldArchive.h"
#include "World.h"
#include <Core/Asset/AssetManager.h>
#include <Core/CoreDefines.h>
#include <Core/Application.h>	

#include <fstream>
#include <nlohmann/json.hpp>

// using json = nlohmann::json;
using json = nlohmann::ordered_json;

#define NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED(Type, ...)                                   \
	inline void to_json(nlohmann::ordered_json& nlohmann_json_j, const Type& nlohmann_json_t)   \
	{                                                                                           \
		NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                \
	}                                                                                           \
	inline void from_json(const nlohmann::ordered_json& nlohmann_json_j, Type& nlohmann_json_t) \
	{                                                                                           \
		NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__))              \
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

inline void to_json(json& Json, const Transform& InTransform)
{
	ForEachAttribute<Transform>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			Json[Name]		 = Attribute.Get(InTransform);
		});
}
inline void from_json(const json& Json, Transform& OutTransform)
{
	ForEachAttribute<Transform>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			if (Json.contains(Name))
			{
				Attribute.Set(OutTransform, Json[Name].get<decltype(Attribute.GetType())>());
			}
		});
}

inline void to_json(json& Json, const MaterialTexture& InMaterialTexture)
{
	ForEachAttribute<MaterialTexture>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			Json[Name]		 = Attribute.Get(InMaterialTexture);
		});
}
inline void from_json(const json& Json, MaterialTexture& OutMaterialTexture)
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
	OutMaterialTexture.Handle.State = false;
	OutMaterialTexture.Handle.Id	= OutMaterialTexture.HandleId;
}

inline void to_json(json& Json, const Material& InMaterial)
{
	ForEachAttribute<Material>(
		[&](auto&& Attribute)
		{
			const char* Name = Attribute.GetName();
			Json[Name]		 = Attribute.Get(InMaterial);
		});
}
inline void from_json(const json& Json, Material& OutMaterial)
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


namespace Version
{
constexpr int	  Major	   = 1;
constexpr int	  Minor	   = 0;
constexpr int	  Revision = 0;
const std::string String   = std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision);
} // namespace Version

template<typename T>
struct ComponentSerializer
{
	ComponentSerializer(json& Json, Actor Actor)
	{
		if (Actor.HasComponent<T>())
		{
			auto& Component = Actor.GetComponent<T>();

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

void WorldArchive::Save(const std::filesystem::path& Path, World* World)
{
	json Json;
	{
		Json["Version"] = Version::String;

		auto& JsonTextures = Json["Textures"];
		AssetManager::GetTextureCache().EnumerateAsset(
			[&](AssetHandle Handle, Texture* Resource)
			{
				std::filesystem::path AssetPath		   = relative(Resource->Options.Path, Application::ExecutableDirectory);
				auto&				  JsonTexture	   = JsonTextures[AssetPath.string()];
				JsonTexture["Options"]["sRGB"]		   = Resource->Options.sRGB;
				JsonTexture["Options"]["GenerateMips"] = Resource->Options.GenerateMips;
			});

		auto& JsonMeshes = Json["Meshes"];
		AssetManager::GetMeshCache().EnumerateAsset(
			[&](AssetHandle Handle, Mesh* Resource)
			{
				std::filesystem::path AssetPath = relative(Resource->Options.Path, Application::ExecutableDirectory);
				auto&				  Mesh		= JsonMeshes[AssetPath.string()];
			});

		auto& JsonWorld = Json["World"];
		for (auto [i, Actor] : enumerate(World->Actors))
		{
			auto& JsonEntity = JsonWorld[i];

			ComponentSerializer<CoreComponent>(JsonEntity, Actor);
			ComponentSerializer<CameraComponent>(JsonEntity, Actor);
			ComponentSerializer<LightComponent>(JsonEntity, Actor);
			ComponentSerializer<SkyLightComponent>(JsonEntity, Actor);
			ComponentSerializer<StaticMeshComponent>(JsonEntity, Actor);
		}
	}

	std::ofstream ofs(Path);
	ofs << std::setfill('\t') << std::setw(1) << Json << std::endl;
}

template<typename T>
struct ComponentDeserializer
{
	ComponentDeserializer(const json& Json, Actor* Actor)
	{
		if (auto iter = Json.find(GetClass<T>()); iter != Json.end())
		{
			auto& Component = Actor->GetOrAddComponent<T>();

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

void WorldArchive::Load(const std::filesystem::path& Path, World* World)
{
	AssetManager::GetTextureCache().DestroyAll();
	AssetManager::GetMeshCache().DestroyAll();

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
			Actor Actor = World->CreateActor();

			ComponentDeserializer<CoreComponent>(JsonEntity, &Actor);
			ComponentDeserializer<CameraComponent>(JsonEntity, &Actor);
			ComponentDeserializer<LightComponent>(JsonEntity, &Actor);
			ComponentDeserializer<SkyLightComponent>(JsonEntity, &Actor);
			ComponentDeserializer<StaticMeshComponent>(JsonEntity, &Actor);

			if (Actor.HasComponent<SkyLightComponent>())
			{
				auto& SkyLight		  = Actor.GetComponent<SkyLightComponent>();
				SkyLight.Handle.Type  = AssetType::Texture;
				SkyLight.Handle.State = false;
				SkyLight.Handle.Id	  = SkyLight.HandleId;
			}

			if (Actor.HasComponent<StaticMeshComponent>())
			{
				auto& StaticMesh		= Actor.GetComponent<StaticMeshComponent>();
				StaticMesh.Handle.Type	= AssetType::Mesh;
				StaticMesh.Handle.State = false;
				StaticMesh.Handle.Id	= StaticMesh.HandleId;
			}
		}
	}
}
