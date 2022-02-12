#include "MitsubaLoader.h"
#include <ranges>
#include <regex>

#include "Globals.h"

std::vector<std::string> Tokenize(std::string String, std::regex Delimiter)
{
	std::sregex_token_iterator Iter{ String.begin(), String.end(), Delimiter, -1 };
	std::vector<std::string>   Tokens{ Iter, {} };

	Tokens.erase(std::remove_if(Tokens.begin(), Tokens.end(), [](const std::string& String)
								{
									return String.size() == 0;
								}),
				 Tokens.end());

	return Tokens;
}

using namespace System::Xml;

static std::filesystem::path					 ParentPath;
static std::unordered_map<std::string, Material> MaterialMap;

struct PathIntegratorParameters
{
	i32 Depth;
} PathIntegratorArgs;

i32 GetIntegerProperty(const XmlNode* Parent, std::string_view Name, i32 DefaultValue)
{
	if (const XmlNode* Property = Parent->GetChild(
			[Name](const XmlNode& Node)
			{
				if (Node.Tag == "integer")
				{
					if (auto Attribute = Node.GetAttributeByName("name");
						Attribute)
					{
						return Attribute->Value == Name;
					}
				}
				return false;
			});
		Property)
	{
		if (auto Attribute = Property->GetAttributeByName("value");
			Attribute)
		{
			return Attribute->GetValue<i32>();
		}
	}

	return DefaultValue;
}

f32 GetFloatProperty(const XmlNode* Parent, std::string_view Name, f32 DefaultValue)
{
	if (const XmlNode* Property = Parent->GetChild(
			[Name](const XmlNode& Node)
			{
				if (Node.Tag == "float")
				{
					if (auto Attribute = Node.GetAttributeByName("name");
						Attribute)
					{
						return Attribute->Value == Name;
					}
				}
				return false;
			});
		Property)
	{
		if (auto Attribute = Property->GetAttributeByName("value");
			Attribute)
		{
			return Attribute->GetValue<f32>();
		}
	}

	return DefaultValue;
}

Vec3f GetRgbProperty(const XmlNode* Parent, std::string_view Name)
{
	if (const XmlNode* Property = Parent->GetChild(
			[Name](const XmlNode& Node)
			{
				if (Node.Tag == "rgb")
				{
					if (auto Attribute = Node.GetAttributeByName("name");
						Attribute)
					{
						return Attribute->Value == Name;
					}
				}
				return false;
			});
		Property)
	{
		if (auto Attribute = Property->GetAttributeByName("value");
			Attribute)
		{
			auto Tokens = Tokenize(std::string(Attribute->Value.begin(), Attribute->Value.end()), std::regex(", "));
			assert(Tokens.size() == 3);
			size_t i = 0;
			Vec3f  Value;
			for (const auto& Token : Tokens)
			{
				Value[i++] = std::stof(Token);
			}
			return Value;
		}
	}

	return {};
}

std::string GetStringProperty(const XmlNode* Parent, std::string_view Name)
{
	if (const XmlNode* Property = Parent->GetChild(
			[Name](const XmlNode& Node)
			{
				if (Node.Tag == "string")
				{
					if (auto Attribute = Node.GetAttributeByName("name");
						Attribute)
					{
						return Attribute->Value == Name;
					}
				}
				return false;
			});
		Property)
	{
		if (auto Attribute = Property->GetAttributeByName("value");
			Attribute)
		{
			return std::string(Attribute->Value.begin(), Attribute->Value.end());
		}
	}

	return {};
}

DirectX::XMFLOAT4X4 GetMatrixFromAttribute(const XmlNode* Node)
{
	float Elements[16] = {};

	if (auto Attribute = Node->GetAttributeByName("value");
		Attribute)
	{
		auto Tokens = Tokenize(std::string(Attribute->Value.begin(), Attribute->Value.end()), std::regex(" "));
		assert(Tokens.size() == 16);
		size_t i = 0;
		for (const auto& Token : Tokens)
		{
			Elements[i++] = std::stof(Token);
		}
	}

	DirectX::XMFLOAT4X4 Temp = DirectX::XMFLOAT4X4(Elements);
	XMStoreFloat4x4(&Temp, XMMatrixTranspose(XMLoadFloat4x4(&Temp)));
	return Temp;
}

DirectX::XMFLOAT4X4 ParseTransformNode(const XmlNode* Node)
{
	DirectX::XMMATRIX Matrix = DirectX::XMMatrixIdentity();

	if (const XmlNode* TransformNode = Node->GetChildByTag("transform");
		TransformNode)
	{
		for (auto Child : TransformNode->Children)
		{
			if (Child.Tag == "matrix")
			{
				DirectX::XMFLOAT4X4 World = GetMatrixFromAttribute(&Child);
				/*World(3, 0) = -World(3, 0);
				World(3, 1) = -World(3, 1);
				World(3, 2) = -World(3, 2);*/

				Matrix = XMLoadFloat4x4(&World) * Matrix;
			}
		}
	}

	DirectX::XMFLOAT4X4 World;
	XMStoreFloat4x4(&World, Matrix);
	return World;
}

Material ParseMaterial(const XmlNode* Node)
{
	const XmlNode* Bsdf = Node;
	if (Node->Tag != "bsdf")
	{
		auto Ref = Node->GetChildByTag("ref");
		if (Ref)
		{
			auto Id = Ref->GetAttributeByName("id");
			if (Id)
			{
				auto Iter = MaterialMap.find(std::string(Id->Value.begin(), Id->Value.end()));
				if (Iter != MaterialMap.end())
				{
					return Iter->second;
				}
				else
				{
					return Material{};
				}
			}
		}

		Bsdf = Node->GetChildByTag("bsdf");
		if (!Bsdf)
		{
			return Material{};
		}
	}

	auto			 Name	  = Node->GetAttributeByName("id");
	std::string_view BsdfType = Bsdf->GetAttributeByName("type")->Value;

	// Keep peeling back nested BSDFs, we only care about the innermost one
	while (BsdfType == "twosided" ||
		   BsdfType == "mask" ||
		   BsdfType == "bumpmap" ||
		   BsdfType == "coating")
	{
		auto BsdfChild = Bsdf->GetChildByTag("bsdf");
		if (BsdfChild)
		{
			Bsdf = BsdfChild;
		}

		BsdfType = Bsdf->GetAttributeByName("type")->Value;
		if (!Name)
		{
			Name = Bsdf->GetAttributeByName("type");
		}
	}

	std::string Id;
	if (Name)
	{
		Id = std::string(Name->Value.begin(), Name->Value.end());
	}
	else
	{
		Id = "Material";
	}

	Material Material;
	if (BsdfType == "plastic" || BsdfType == "roughplastic")
	{
		Vec3f Albedo	   = GetRgbProperty(Bsdf, "diffuseReflectance");
		Material.BaseColor = { Albedo.x, Albedo.y, Albedo.z };
	}
	if (BsdfType == "thindielectric" || BsdfType == "dielectric" || BsdfType == "roughdielectric")
	{
		Material.BSDFType = EBSDFTypes::Glass;
		Material.EtaA	  = GetFloatProperty(Bsdf, "extIOR", 1.0f);
		Material.EtaB	  = GetFloatProperty(Bsdf, "intIOR", 1.0f);
	}
	if (BsdfType == "conductor")
	{
		Material.BSDFType  = EBSDFTypes::Disney;
		Material.Metallic  = 1.0f;
		Material.Specular  = 1.0f;
		Material.Roughness = 0.0f;
	}

	MaterialMap[Id] = Material;
	return Material;
}

void MitsubaLoader::Load(const std::filesystem::path& Path, Asset::AssetManager* AssetManager, World* World)
{
	ParentPath = Path.parent_path();

	FileStream Stream(Path, FileMode::Open, FileAccess::Read);
	XmlReader  Reader(Stream);

	XmlNode		   RootNode	 = Reader.ParseRootNode();
	const XmlNode* SceneNode = RootNode.GetChildByTag("scene");
	if (!SceneNode)
	{
		throw std::exception("Invalid scene");
	}
	ParseNode(SceneNode, AssetManager, World);
}

void MitsubaLoader::ParseNode(const XmlNode* Node, Asset::AssetManager* AssetManager, World* World)
{
	if (Node->Tag == "integrator")
	{
		PathIntegratorArgs.Depth = GetIntegerProperty(Node, "maxDepth", 8);
	}
	if (Node->Tag == "sensor")
	{
		float Fov				  = GetFloatProperty(Node, "fov", 90.0f);
		World->ActiveCamera->FoVY = Fov;

		auto  CameraTransform = ParseTransformNode(Node);
		auto& Transform		  = World->ActiveCameraActor.GetComponent<CoreComponent>().Transform;
		Transform.SetTransform(XMLoadFloat4x4(&CameraTransform));
		Transform.Position = {
			-Transform.Position.x,
			Transform.Position.y,
			-Transform.Position.z
		};
		Transform.Rotate(0.0f, 180.0f, 0.0f);
	}
	if (Node->Tag == "bsdf")
	{
		ParseMaterial(Node);
	}
	if (Node->Tag == "shape")
	{
		Material Material = ParseMaterial(Node);

		auto Attribute = Node->GetAttributeByName("type");
		if (Attribute)
		{
			if (Attribute->Value == "obj" || Attribute->Value == "ply")
			{
				Asset::MeshImportOptions Options = {};
				Options.Path					 = ParentPath / GetStringProperty(Node, "filename");
				Options.Matrix					 = ParseTransformNode(Node);
				auto Handles					 = AssetManager->LoadMesh(Options);
				assert(Handles.size() == 1);
				auto Mesh  = AssetManager->GetMeshRegistry().GetAsset(Handles[0]);
				Mesh->Name = Options.Path.filename().string();

				auto  Entity		= World->CreateActor(Mesh->Name);
				auto& StaticMesh	= Entity.AddComponent<StaticMeshComponent>();
				StaticMesh.Handle	= Handles[0];
				StaticMesh.Material = Material;
			}
		}
	}

	for (const auto& Child : Node->Children)
	{
		ParseNode(&Child, AssetManager, World);
	}
}
