#include "pch.h"
#include "SceneParser.h"

#include "Scene.h"
#include "Entity.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "../AssetManager.h"

namespace Version
{
constexpr int	  Major	   = 1;
constexpr int	  Minor	   = 0;
constexpr int	  Revision = 0;
const std::string String   = std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision);
} // namespace Version

YAML::Emitter& operator<<(YAML::Emitter& Emitter, const DirectX::XMFLOAT2& Float2)
{
	Emitter << YAML::Flow;
	Emitter << YAML::BeginSeq << Float2.x << Float2.y << YAML::EndSeq;
	return Emitter;
}

YAML::Emitter& operator<<(YAML::Emitter& Emitter, const DirectX::XMFLOAT3& Float3)
{
	Emitter << YAML::Flow;
	Emitter << YAML::BeginSeq << Float3.x << Float3.y << Float3.z << YAML::EndSeq;
	return Emitter;
}

YAML::Emitter& operator<<(YAML::Emitter& Emitter, const DirectX::XMFLOAT4& Float4)
{
	Emitter << YAML::Flow;
	Emitter << YAML::BeginSeq << Float4.x << Float4.y << Float4.z << Float4.w << YAML::EndSeq;
	return Emitter;
}

template<is_component T>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const T& Component)
{
	return Emitter;
}

template<>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const Tag& Tag)
{
	Emitter << YAML::Key << "Tag";
	Emitter << YAML::BeginMap;
	{
		Emitter << YAML::Key << "Name" << YAML::Value << Tag.Name;
	}
	Emitter << YAML::EndMap;
	return Emitter;
}

template<>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const Transform& Transform)
{
	Emitter << YAML::Key << "Transform";
	Emitter << YAML::BeginMap;
	{
		Emitter << YAML::Key << "Position" << YAML::Value << Transform.Position;
		Emitter << YAML::Key << "Scale" << YAML::Value << Transform.Scale;
		Emitter << YAML::Key << "Orientation" << YAML::Value << Transform.Orientation;
	}
	Emitter << YAML::EndMap;
	return Emitter;
}

template<>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const MeshFilter& MeshFilter)
{
	Emitter << YAML::Key << "Mesh Filter";
	Emitter << YAML::BeginMap;
	{
		std::string relativePath =
			MeshFilter.Mesh
				? std::filesystem::relative(MeshFilter.Mesh->Metadata.Path, Application::ExecutableDirectory).string()
				: "NULL";

		Emitter << YAML::Key << "Name" << relativePath;
	}
	Emitter << YAML::EndMap;
	return Emitter;
}

template<>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const MeshRenderer& MeshRenderer)
{
	Emitter << YAML::Key << "Mesh Renderer";
	Emitter << YAML::BeginMap;
	{
		auto& material = MeshRenderer.Material;
		Emitter << YAML::Key << "Material";
		Emitter << YAML::BeginMap;
		{
			Emitter << YAML::Key << "BSDFType" << material.BSDFType;

			Emitter << YAML::Key << "baseColor" << material.baseColor;
			Emitter << YAML::Key << "metallic" << material.metallic;
			Emitter << YAML::Key << "subsurface" << material.subsurface;
			Emitter << YAML::Key << "specular" << material.specular;
			Emitter << YAML::Key << "roughness" << material.roughness;
			Emitter << YAML::Key << "specularTint" << material.specularTint;
			Emitter << YAML::Key << "anisotropic" << material.anisotropic;
			Emitter << YAML::Key << "sheen" << material.sheen;
			Emitter << YAML::Key << "sheenTint" << material.sheenTint;
			Emitter << YAML::Key << "clearcoat" << material.clearcoat;
			Emitter << YAML::Key << "clearcoatGloss" << material.clearcoatGloss;

			Emitter << YAML::Key << "T" << material.T;
			Emitter << YAML::Key << "etaA" << material.etaA;
			Emitter << YAML::Key << "etaB" << material.etaB;

			const char* textureTypes[TextureTypes::NumTextureTypes] = { "Albedo", "Normal", "Roughness", "Metallic" };
			for (int i = 0; i < TextureTypes::NumTextureTypes; ++i)
			{
				const auto& texture = material.Textures[i];

				std::string relativePath =
					texture
						? std::filesystem::relative(texture->Metadata.Path, Application::ExecutableDirectory).string()
						: "NULL";

				Emitter << YAML::Key << textureTypes[i] << relativePath;
			}
		}
		Emitter << YAML::EndMap;
	}
	Emitter << YAML::EndMap;
	return Emitter;
}

template<>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const Light& Light)
{
	Emitter << YAML::Key << "Light";
	Emitter << YAML::BeginMap;
	{
		Emitter << YAML::Key << "Type" << Light.Type;
		Emitter << YAML::Key << "I" << Light.I;
		Emitter << YAML::Key << "Width" << Light.Width;
		Emitter << YAML::Key << "Height" << Light.Height;
	}
	Emitter << YAML::EndMap;
	return Emitter;
}

namespace YAML
{
template<>
struct convert<DirectX::XMFLOAT2>
{
	static auto encode(const DirectX::XMFLOAT2& Float2)
	{
		Node node;
		node.push_back(Float2.x);
		node.push_back(Float2.y);
		return node;
	}

	static bool decode(const Node& Node, DirectX::XMFLOAT2& Float2)
	{
		if (!Node.IsSequence() || Node.size() != 2)
		{
			return false;
		}

		Float2.x = Node[0].as<float>();
		Float2.y = Node[1].as<float>();
		return true;
	}
};

template<>
struct convert<DirectX::XMFLOAT3>
{
	static auto encode(const DirectX::XMFLOAT3& Float3)
	{
		Node node;
		node.push_back(Float3.x);
		node.push_back(Float3.y);
		node.push_back(Float3.z);
		return node;
	}

	static bool decode(const Node& Node, DirectX::XMFLOAT3& Float3)
	{
		if (!Node.IsSequence() || Node.size() != 3)
		{
			return false;
		}

		Float3.x = Node[0].as<float>();
		Float3.y = Node[1].as<float>();
		Float3.z = Node[2].as<float>();
		return true;
	}
};

template<>
struct convert<DirectX::XMFLOAT4>
{
	static auto encode(const DirectX::XMFLOAT4& Float4)
	{
		Node node;
		node.push_back(Float4.x);
		node.push_back(Float4.y);
		node.push_back(Float4.z);
		node.push_back(Float4.w);
		return node;
	}

	static bool decode(const Node& Node, DirectX::XMFLOAT4& Float4)
	{
		if (!Node.IsSequence() || Node.size() != 4)
		{
			return false;
		}

		Float4.x = Node[0].as<float>();
		Float4.y = Node[1].as<float>();
		Float4.z = Node[2].as<float>();
		Float4.w = Node[3].as<float>();
		return true;
	}
};
} // namespace YAML

static void SerializeCamera(YAML::Emitter& Emitter, const Camera& Camera)
{
	Emitter << YAML::Key << "Camera";
	Emitter << YAML::BeginMap;
	{
		DirectX::XMFLOAT2 clippingPlanes = { Camera.NearZ, Camera.FarZ };

		Emitter << Camera.Transform;
		Emitter << YAML::Key << "Field of View" << YAML::Value << Camera.FoVY;
		Emitter << YAML::Key << "Clipping Planes" << YAML::Value << clippingPlanes;
		Emitter << YAML::Key << "Focal Length" << YAML::Value << Camera.FocalLength;
		Emitter << YAML::Key << "Relative Aperture" << YAML::Value << Camera.RelativeAperture;
		Emitter << YAML::Key << "Shutter Time" << YAML::Value << Camera.ShutterTime;
		Emitter << YAML::Key << "Sensor Sensitivity" << YAML::Value << Camera.SensorSensitivity;
	}
	Emitter << YAML::EndMap;
}

static void SerializeImages(YAML::Emitter& Emitter)
{
	auto& ImageCache = AssetManager::GetImageCache();

	Emitter << YAML::Key << "Images" << YAML::Value << YAML::BeginSeq;
	{
		std::map<std::string, AssetHandle<Asset::Image>> sortedImages;
		ImageCache.Each_ThreadSafe(
			[&](UINT64 Key, AssetHandle<Asset::Image> Resource)
			{
				sortedImages.insert({ Resource->Metadata.Path.string(), Resource });
			});

		for (auto& SortedImage : sortedImages)
		{
			Emitter << YAML::BeginMap;
			{
				Emitter << YAML::Key << "Image"
						<< std::filesystem::relative(
							   SortedImage.second->Metadata.Path,
							   Application::ExecutableDirectory)
							   .string();
				Emitter << YAML::Key << "Metadata";
				Emitter << YAML::BeginMap;
				{
					Emitter << YAML::Key << "sRGB" << SortedImage.second->Metadata.sRGB;
				}
				Emitter << YAML::EndMap;
			}
			Emitter << YAML::EndMap;
		}
	}
	Emitter << YAML::EndSeq;
}

static void SerializeMeshes(YAML::Emitter& Emitter)
{
	auto& MeshCache = AssetManager::GetMeshCache();

	Emitter << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;
	{
		std::map<std::string, AssetHandle<Asset::Mesh>> sortedMeshes;
		MeshCache.Each_ThreadSafe(
			[&](UINT64 Key, AssetHandle<Asset::Mesh> Resource)
			{
				sortedMeshes.insert({ Resource->Metadata.Path.string(), Resource });
			});

		for (auto& SortedMeshe : sortedMeshes)
		{
			Emitter << YAML::BeginMap;
			{
				Emitter << YAML::Key << "Mesh" << YAML::Value
						<< std::filesystem::relative(
							   SortedMeshe.second->Metadata.Path,
							   Application::ExecutableDirectory)
							   .string();
				Emitter << YAML::Key << "Metadata";
				Emitter << YAML::BeginMap;
				{
					Emitter << YAML::Key << "KeepGeometryInRAM" << SortedMeshe.second->Metadata.KeepGeometryInRAM;
				}
				Emitter << YAML::EndMap;
			}
			Emitter << YAML::EndMap;
		}
	}
	Emitter << YAML::EndSeq;
}

template<is_component T>
void SerializeComponent(YAML::Emitter& Emitter, Entity Entity)
{
	if (Entity.HasComponent<T>())
	{
		auto& Component = Entity.GetComponent<T>();

		Emitter << Component;
	}
}

static void SerializeEntity(YAML::Emitter& Emitter, Entity Entity)
{
	Emitter << YAML::BeginMap;
	{
		Emitter << YAML::Key << "Entity" << YAML::Value << std::to_string(typeid(Entity).hash_code());

		SerializeComponent<Tag>(Emitter, Entity);
		SerializeComponent<Transform>(Emitter, Entity);
		SerializeComponent<MeshFilter>(Emitter, Entity);
		SerializeComponent<MeshRenderer>(Emitter, Entity);
		SerializeComponent<Light>(Emitter, Entity);
	}
	Emitter << YAML::EndMap;
}

void SceneParser::Save(const std::filesystem::path& Path, Scene* pScene)
{
	YAML::Emitter emitter;
	emitter << YAML::BeginMap;
	{
		emitter << YAML::Key << "Version" << YAML::Value << Version::String;
		emitter << YAML::Key << "Scene" << YAML::Value << Path.filename().string();

		SerializeCamera(emitter, pScene->Camera);
		SerializeImages(emitter);
		SerializeMeshes(emitter);

		emitter << YAML::Key << "WorldBegin" << YAML::Value << YAML::BeginSeq;
		{
			pScene->Registry.each(
				[&](auto Handle)
				{
					Entity entity(Handle, pScene);
					if (!entity)
					{
						return;
					}

					SerializeEntity(emitter, entity);
				});
		}
		emitter << YAML::EndSeq;
	}
	emitter << YAML::EndMap;

	std::ofstream fout(Path);
	fout << emitter.c_str();
}

static void DeserializeCamera(const YAML::Node& Node, Scene* pScene)
{
	auto transform = Node["Transform"];

	Camera camera = {};

	camera.Transform.Position	 = transform["Position"].as<DirectX::XMFLOAT3>();
	camera.Transform.Scale		 = transform["Scale"].as<DirectX::XMFLOAT3>();
	camera.Transform.Orientation = transform["Orientation"].as<DirectX::XMFLOAT4>();

	camera.FoVY		   = Node["Field of View"].as<float>();
	camera.AspectRatio = 1.0f;
	camera.NearZ	   = Node["Clipping Planes"].as<DirectX::XMFLOAT2>().x;
	camera.FarZ		   = Node["Clipping Planes"].as<DirectX::XMFLOAT2>().y;

	camera.FocalLength		 = Node["Focal Length"].as<float>();
	camera.RelativeAperture	 = Node["Relative Aperture"].as<float>();
	camera.ShutterTime		 = Node["Shutter Time"].as<float>();
	camera.SensorSensitivity = Node["Sensor Sensitivity"].as<float>();

	pScene->Camera		   = camera;
	pScene->PreviousCamera = camera;
}

static void DeserializeImage(const YAML::Node& Node)
{
	auto Path	  = Node["Image"].as<std::string>();
	Path		  = (Application::ExecutableDirectory / Path).string();
	auto Metadata = Node["Metadata"];

	bool sRGB = Metadata["sRGB"].as<bool>();

	AssetManager::AsyncLoadImage(Path, sRGB);
}

static void DeserializeMesh(const YAML::Node& Node)
{
	auto Path	  = Node["Mesh"].as<std::string>();
	Path		  = (Application::ExecutableDirectory / Path).string();
	auto Metadata = Node["Metadata"];

	bool keepGeometryInRAM = Metadata["KeepGeometryInRAM"].as<bool>();

	AssetManager::AsyncLoadMesh(Path, keepGeometryInRAM);
}

template<is_component T, typename DeserializeFunction>
void DeserializeComponent(const YAML::Node& Node, Entity* pEntity, DeserializeFunction Deserialize)
{
	if (Node)
	{
		auto& component = pEntity->GetOrAddComponent<T>();

		Deserialize(Node, component);
	}
}

static void DeserializeEntity(const YAML::Node& Node, Scene* pScene)
{
	// Tag component have to exist
	auto Name = Node["Tag"]["Name"].as<std::string>();

	// Create entity after getting our tag component
	Entity Entity = pScene->CreateEntity(Name);

	// TODO: Come up with some template meta programming for serialize/deserialize
	DeserializeComponent<Transform>(
		Node["Transform"],
		&Entity,
		[](auto& Node, auto& Transform)
		{
			Transform.Position	  = Node["Position"].as<DirectX::XMFLOAT3>();
			Transform.Scale		  = Node["Scale"].as<DirectX::XMFLOAT3>();
			Transform.Orientation = Node["Orientation"].as<DirectX::XMFLOAT4>();
		});

	DeserializeComponent<MeshFilter>(
		Node["Mesh Filter"],
		&Entity,
		[](auto& Node, auto& MeshFilter)
		{
			auto String = Node["Name"].as<std::string>();
			if (String != "NULL")
			{
				String = (Application::ExecutableDirectory / String).string();

				MeshFilter.Key = CityHash64(String.data(), String.size());
			}
		});

	DeserializeComponent<MeshRenderer>(
		Node["Mesh Renderer"],
		&Entity,
		[](auto& Node, MeshRenderer& MeshRenderer)
		{
			auto& Material				   = Node["Material"];
			MeshRenderer.Material.BSDFType = Material["BSDFType"].as<int>();

			MeshRenderer.Material.baseColor		 = Material["baseColor"].as<DirectX::XMFLOAT3>();
			MeshRenderer.Material.metallic		 = Material["metallic"].as<float>();
			MeshRenderer.Material.subsurface	 = Material["subsurface"].as<float>();
			MeshRenderer.Material.specular		 = Material["specular"].as<float>();
			MeshRenderer.Material.roughness		 = Material["roughness"].as<float>();
			MeshRenderer.Material.specularTint	 = Material["specularTint"].as<float>();
			MeshRenderer.Material.anisotropic	 = Material["anisotropic"].as<float>();
			MeshRenderer.Material.sheen			 = Material["sheen"].as<float>();
			MeshRenderer.Material.sheenTint		 = Material["sheenTint"].as<float>();
			MeshRenderer.Material.clearcoat		 = Material["clearcoat"].as<float>();
			MeshRenderer.Material.clearcoatGloss = Material["clearcoatGloss"].as<float>();

			MeshRenderer.Material.T	   = Material["T"].as<DirectX::XMFLOAT3>();
			MeshRenderer.Material.etaA = Material["etaA"].as<float>();
			MeshRenderer.Material.etaB = Material["etaB"].as<float>();

			const char* TextureTypes[TextureTypes::NumTextureTypes] = { "Albedo", "Normal", "Roughness", "Metallic" };
			for (int i = 0; i < TextureTypes::NumTextureTypes; ++i)
			{
				auto String = Material[TextureTypes[i]].as<std::string>();
				if (String != "NULL")
				{
					String = (Application::ExecutableDirectory / String).string();

					MeshRenderer.Material.TextureKeys[i] = CityHash64(String.data(), String.size());
				}
			}
		});

	DeserializeComponent<Light>(
		Node["Light"],
		&Entity,
		[](auto& Node, auto& Light)
		{
			Light.Type	 = static_cast<Light::EType>(Node["Type"].as<int>());
			Light.I		 = Node["I"].as<DirectX::XMFLOAT3>();
			Light.Width	 = Node["Width"].as<float>();
			Light.Height = Node["Height"].as<float>();
		});
}

void SceneParser::Load(const std::filesystem::path& Path, Scene* pScene)
{
	pScene->Clear();

	AssetManager::GetImageCache().DestroyAll();
	AssetManager::GetMeshCache().DestroyAll();

	std::ifstream	  fin(Path);
	std::stringstream ss;
	ss << fin.rdbuf();

	auto data = YAML::Load(ss.str());

	if (!data["Version"])
	{
		throw std::exception("Invalid file");
	}

	auto version = data["Version"].as<std::string>();
	if (version != Version::String)
	{
		throw std::exception("Invalid version");
	}

	auto camera = data["Camera"];
	if (!camera)
	{
		throw std::exception("Invalid camera");
	}

	DeserializeCamera(camera, pScene);

	auto images = data["Images"];
	if (images)
	{
		for (auto image : images)
		{
			DeserializeImage(image);
		}
	}

	auto meshes = data["Meshes"];
	if (meshes)
	{
		for (auto mesh : meshes)
		{
			DeserializeMesh(mesh);
		}
	}

	auto world = data["WorldBegin"];
	if (world)
	{
		for (auto entity : world)
		{
			DeserializeEntity(entity, pScene);
		}
	}
}
