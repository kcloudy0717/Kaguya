#pragma once
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
