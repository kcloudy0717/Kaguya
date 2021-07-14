#include "SceneParser.h"

#include "World.h"
#include "Entity.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

#include <Graphics/AssetManager.h>

using namespace DirectX;

namespace Version
{
constexpr int	  Major	   = 1;
constexpr int	  Minor	   = 0;
constexpr int	  Revision = 0;
const std::string String   = std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision);
} // namespace Version

struct YAMLScopedMapObject
{
	YAMLScopedMapObject(YAML::Emitter& Emitter)
		: Emitter(Emitter)
	{
		Emitter << YAML::BeginMap;
	}

	~YAMLScopedMapObject() { Emitter << YAML::EndMap; }

	YAML::Emitter& Emitter;
};

#define YAMLConcatenate(a, b)				 a##b
#define YAMLGetScopedEventVariableName(a, b) YAMLConcatenate(a, b)
#define YAMLScopedMap(context)				 YAMLScopedMapObject YAMLGetScopedEventVariableName(scopedMap, __LINE__)(context)

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

YAML::Emitter& operator<<(YAML::Emitter& Emitter, ELightTypes LightType)
{
	return Emitter << (int)LightType;
}

YAML::Emitter& operator<<(YAML::Emitter& Emitter, EBSDFTypes BSDFType)
{
	return Emitter << (int)BSDFType;
}

YAML::Emitter& operator<<(YAML::Emitter& Emitter, const MaterialTexture& MaterialTexture)
{
	return Emitter << MaterialTexture.Path;
}

template<typename T>
YAML::Emitter& operator<<(YAML::Emitter& Emitter, const T& Type)
{
	YAMLScopedMap(Emitter);
	ForEachAttribute<T>(
		[&](auto&& Attribute)
		{
			Emitter << YAML::Key << Attribute.GetName() << YAML::Value << Attribute.Get(Type);
		});
	return Emitter;
}

template<typename T>
struct ComponentSerializer
{
	ComponentSerializer(YAML::Emitter& Emitter, Entity Entity)
	{
		if (Entity.HasComponent<T>())
		{
			auto& Component = Entity.GetComponent<T>();

			Emitter << YAML::Key << GetClass<T>();
			YAMLScopedMap(Emitter);
			ForEachAttribute<T>(
				[&](auto&& Attribute)
				{
					Emitter << YAML::Key << Attribute.GetName() << YAML::Value << Attribute.Get(Component);
				});
		}
	}
};

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

static void SerializeImages(YAML::Emitter& Emitter)
{
	auto& ImageCache = AssetManager::GetImageCache();

	Emitter << YAML::Key << "Images" << YAML::Value << YAML::BeginSeq;
	{
		std::map<std::string, AssetHandle<Asset::Image>> SortedImages;
		ImageCache.Each_ThreadSafe(
			[&](UINT64 Key, AssetHandle<Asset::Image> Resource)
			{
				SortedImages.insert({ Resource->Metadata.Path.string(), Resource });
			});

		for (auto& SortedImage : SortedImages)
		{
			YAMLScopedMap(Emitter);
			Emitter << YAML::Key << "Image"
					<< std::filesystem::relative(SortedImage.second->Metadata.Path, Application::ExecutableDirectory)
						   .string();
			Emitter << YAML::Key << "Metadata";
			{
				YAMLScopedMap(Emitter);
				Emitter << YAML::Key << "sRGB" << SortedImage.second->Metadata.sRGB;
			}
		}
	}
	Emitter << YAML::EndSeq;
}

static void SerializeMeshes(YAML::Emitter& Emitter)
{
	auto& MeshCache = AssetManager::GetMeshCache();

	Emitter << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;
	{
		std::map<std::string, AssetHandle<Asset::Mesh>> SortedMeshes;
		MeshCache.Each_ThreadSafe(
			[&](UINT64 Key, AssetHandle<Asset::Mesh> Resource)
			{
				SortedMeshes.insert({ Resource->Metadata.Path.string(), Resource });
			});

		for (auto& SortedMeshe : SortedMeshes)
		{
			YAMLScopedMap(Emitter);
			Emitter << YAML::Key << "Mesh" << YAML::Value
					<< std::filesystem::relative(SortedMeshe.second->Metadata.Path, Application::ExecutableDirectory)
						   .string();
			Emitter << YAML::Key << "Metadata";
			{
				YAMLScopedMap(Emitter);
				Emitter << YAML::Key << "KeepGeometryInRAM" << SortedMeshe.second->Metadata.KeepGeometryInRAM;
			}
		}
	}
	Emitter << YAML::EndSeq;
}

static void SerializeEntity(YAML::Emitter& Emitter, Entity Entity)
{
	YAMLScopedMap(Emitter);
	Emitter << YAML::Key << "Entity" << YAML::Value << std::to_string(typeid(Entity).hash_code());

	ComponentSerializer<Tag>(Emitter, Entity);
	ComponentSerializer<Transform>(Emitter, Entity);
	ComponentSerializer<Camera>(Emitter, Entity);
	ComponentSerializer<Light>(Emitter, Entity);

	ComponentSerializer<MeshFilter>(Emitter, Entity);
	ComponentSerializer<MeshRenderer>(Emitter, Entity);

	ComponentSerializer<BoxCollider>(Emitter, Entity);
	ComponentSerializer<CapsuleCollider>(Emitter, Entity);

	// ComponentSerializer<CharacterController>(Emitter, Entity);
	// ComponentSerializer<NativeScript>(Emitter, Entity);
	// ComponentSerializer<StaticRigidBody>(Emitter, Entity);
	// ComponentSerializer<DynamicRigidBody>(Emitter, Entity);
	//
	// ComponentSerializer<MeshCollider>(Emitter, Entity);
}

void SceneParser::Save(const std::filesystem::path& Path, World* pWorld)
{
	YAML::Emitter Emitter;
	{
		YAMLScopedMap(Emitter);
		Emitter << YAML::Key << "Version" << YAML::Value << Version::String;
		Emitter << YAML::Key << "World" << YAML::Value << Path.filename().string();

		SerializeImages(Emitter);
		SerializeMeshes(Emitter);

		Emitter << YAML::Key << "WorldBegin" << YAML::Value << YAML::BeginSeq;
		{
			pWorld->Registry.each(
				[&](auto Handle)
				{
					Entity Entity(Handle, pWorld);
					SerializeEntity(Emitter, Entity);
				});
		}
		Emitter << YAML::EndSeq;
	}

	std::ofstream fout(Path);
	fout << Emitter.c_str();
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
		auto& Component = pEntity->GetOrAddComponent<T>();

		Deserialize(Node, Component);
	}
}

static void DeserializeEntity(const YAML::Node& Node, World* pWorld)
{
	// Tag component have to exist
	auto Name = Node["Tag"]["Name"].as<std::string>();

	// Create entity after getting our tag component
	Entity Entity = pWorld->CreateEntity(Name);

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

	DeserializeComponent<Camera>(
		Node["Camera"],
		&Entity,
		[](auto& Node, auto& Camera)
		{
			Camera.FoVY		   = Node["Field of View"].as<float>();
			Camera.AspectRatio = 16.0f / 9.0f;

			DirectX::XMFLOAT2 ClippingPlanes = Node["Clipping Planes"].as<DirectX::XMFLOAT2>();
			Camera.NearZ					 = ClippingPlanes.x;
			Camera.FarZ						 = ClippingPlanes.y;

			Camera.FocalLength		= Node["Focal Length"].as<float>();
			Camera.RelativeAperture = Node["Relative Aperture"].as<float>();
		});

	DeserializeComponent<Light>(
		Node["Light"],
		&Entity,
		[](auto& Node, auto& Light)
		{
			Light.Type	 = static_cast<ELightTypes>(Node["Type"].as<int>());
			Light.I		 = Node["I"].as<DirectX::XMFLOAT3>();
			Light.Width	 = Node["Width"].as<float>();
			Light.Height = Node["Height"].as<float>();
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
			MeshRenderer.Material.BSDFType = (EBSDFTypes)Material["BSDFType"].as<int>();

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

			auto String = Material["Albedo"].as<std::string>();
			if (String != "NULL")
			{
				String = (Application::ExecutableDirectory / String).string();

				MeshRenderer.Material.Albedo.Key = CityHash64(String.data(), String.size());
			}
		});

	DeserializeComponent<BoxCollider>(
		Node["Box Collider"],
		&Entity,
		[](auto& Node, BoxCollider& BoxCollider)
		{
			BoxCollider.Extents = Node["Extents"].as<XMFLOAT3>();
		});

	DeserializeComponent<CapsuleCollider>(
		Node["Capsule Collider"],
		&Entity,
		[](auto& Node, CapsuleCollider& CapsuleCollider)
		{
			CapsuleCollider.Radius = Node["Radius"].as<float>();
			CapsuleCollider.Height = Node["Height"].as<float>();
		});
}

void SceneParser::Load(const std::filesystem::path& Path, World* pWorld)
{
	pWorld->Clear();

	AssetManager::GetImageCache().DestroyAll();
	AssetManager::GetMeshCache().DestroyAll();

	std::ifstream	  fin(Path);
	std::stringstream ss;
	ss << fin.rdbuf();

	auto YAMLData = YAML::Load(ss.str());

	if (!YAMLData["Version"])
	{
		throw std::exception("Invalid file");
	}

	auto YAMLVersion = YAMLData["Version"].as<std::string>();
	if (YAMLVersion != Version::String)
	{
		throw std::exception("Invalid version");
	}

	auto YAMLImages = YAMLData["Images"];
	if (YAMLImages)
	{
		for (auto Image : YAMLImages)
		{
			DeserializeImage(Image);
		}
	}

	auto YAMLMeshes = YAMLData["Meshes"];
	if (YAMLMeshes)
	{
		for (auto Mesh : YAMLMeshes)
		{
			DeserializeMesh(Mesh);
		}
	}

	auto YAMLWorld = YAMLData["WorldBegin"];
	if (YAMLWorld)
	{
		for (auto Entity : YAMLWorld)
		{
			auto Name = Entity["Tag"]["Name"].as<std::string>();
			if (Name == "Main Camera")
			{
				continue;
			}

			DeserializeEntity(Entity, pWorld);
		}
	}
}
