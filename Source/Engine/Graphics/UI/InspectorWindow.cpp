#include "InspectorWindow.h"
#include <imgui_internal.h>
#include <Core/Asset/AssetManager.h>

template<typename T, bool IsCoreComponent, typename UIFunction>
static void RenderComponent(const char* Name, Entity Entity, UIFunction Func)
{
	if (Entity.HasComponent<T>())
	{
		bool  IsEdited	= false;
		auto& Component = Entity.GetComponent<T>();

		constexpr ImGuiTreeNodeFlags TreeNodeFlags =
			ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::Separator();
		bool Collapsed = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), TreeNodeFlags, Name);
		ImGui::PopStyleVar();

		ImGui::SameLine(ImGui::GetContentRegionAvail().x - lineHeight * 0.5f);
		if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
		{
			ImGui::OpenPopup("Component Settings");
		}

		bool RemoveComponent = false;
		if (ImGui::BeginPopup("Component Settings"))
		{
			if constexpr (!IsCoreComponent)
			{
				if (ImGui::MenuItem("Remove component"))
				{
					RemoveComponent = true;
				}
			}

			ImGui::EndPopup();
		}

		if (Collapsed)
		{
			IsEdited |= Func(Component);
			ImGui::TreePop();
		}

		if (RemoveComponent)
		{
			Entity.RemoveComponent<T>();
		}

		if (IsEdited || RemoveComponent)
		{
			Entity.OnComponentModified();
		}
	}
}

template<typename T>
static void AddNewComponent(const char* pName, Entity Entity)
{
	if (ImGui::MenuItem(pName))
	{
		if (Entity.HasComponent<T>())
		{
			MessageBoxA(
				nullptr,
				"Cannot add existing component",
				"Add Component Error",
				MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		}
		else
		{
			Entity.AddComponent<T>();
		}
	}
}

static bool EditTransform(const float* ViewMatrix, float* ProjectMatrix, float* TransformMatrix)
{
	static bool				   UseSnap				 = false;
	static ImGuizmo::OPERATION CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	static float			   Snap[3]				 = { 1, 1, 1 };
	static ImGuizmo::MODE	   CurrentGizmoMode		 = ImGuizmo::WORLD;

	ImGui::Text("Operation");

	if (ImGui::RadioButton("Translate", CurrentGizmoOperation == ImGuizmo::TRANSLATE))
	{
		CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", CurrentGizmoOperation == ImGuizmo::ROTATE))
	{
		CurrentGizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", CurrentGizmoOperation == ImGuizmo::SCALE))
	{
		CurrentGizmoOperation = ImGuizmo::SCALE;
	}

	ImGui::Checkbox("Snap", &UseSnap);
	ImGui::SameLine();

	switch (CurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		ImGui::InputFloat3("Snap", &Snap[0]);
		break;
	case ImGuizmo::ROTATE:
		ImGui::InputFloat("Angle Snap", &Snap[0]);
		break;
	case ImGuizmo::SCALE:
		ImGui::InputFloat("Scale Snap", &Snap[0]);
		break;
	default:
		break;
	}

	return ImGuizmo::Manipulate(
		ViewMatrix,
		ProjectMatrix,
		CurrentGizmoOperation,
		CurrentGizmoMode,
		TransformMatrix,
		nullptr,
		UseSnap ? Snap : nullptr);
}

void InspectorWindow::OnRender()
{
	if (SelectedEntity)
	{
		RenderComponent<Tag, true>(
			"Tag",
			SelectedEntity,
			[&](Tag& Component)
			{
				char Buffer[MAX_PATH] = {};
				strcpy_s(Buffer, MAX_PATH, Component.Name.data());
				if (ImGui::InputText("Tag", Buffer, MAX_PATH))
				{
					Component.Name = Buffer;
				}

				return false;
			});

		RenderComponent<Transform, true>(
			"Transform",
			SelectedEntity,
			[&](Transform& Component)
			{
				bool IsEdited = false;

				DirectX::XMFLOAT4X4 World, View, Projection;

				// Dont transpose this
				XMStoreFloat4x4(&World, Component.Matrix());

				float Translation[3], Rotation[3], Scale[3];
				ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<float*>(&World), Translation, Rotation, Scale);
				IsEdited |= RenderFloat3Control("Translation", Translation);
				IsEdited |= RenderFloat3Control("Rotation", Rotation);
				IsEdited |= RenderFloat3Control("Scale", Scale, 1.0f);
				ImGuizmo::RecomposeMatrixFromComponents(Translation, Rotation, Scale, reinterpret_cast<float*>(&World));

				XMStoreFloat4x4(&View, XMLoadFloat4x4(&pWorld->ActiveCamera->View));
				XMStoreFloat4x4(&Projection, XMLoadFloat4x4(&pWorld->ActiveCamera->Projection));

				// If we have edited the transform, update it and mark it as dirty so it will be updated on the GPU side
				IsEdited |= EditTransform(
					reinterpret_cast<float*>(&View),
					reinterpret_cast<float*>(&Projection),
					reinterpret_cast<float*>(&World));

				if (IsEdited)
				{
					Component.SetTransform(XMLoadFloat4x4(&World));
				}

				return IsEdited;
			});

		RenderComponent<Camera, false>(
			"Camera",
			SelectedEntity,
			[&](Camera& Component)
			{
				bool IsEdited = false;

				IsEdited |= RenderFloatControl("Vertical FoV", &Component.FoVY, Camera().FoVY, 45.0f, 85.0f);
				IsEdited |= RenderFloatControl("Near", &Component.NearZ, Camera().NearZ, 0.1f, 1.0f);
				IsEdited |= RenderFloatControl("Far", &Component.FarZ, Camera().FarZ, 10.0f, 10000.0f);

				IsEdited |= RenderFloatControl(
					"Movement Speed",
					&Component.MovementSpeed,
					Camera().MovementSpeed,
					1.0f,
					1000.0f);
				IsEdited |=
					RenderFloatControl("Strafe Speed", &Component.StrafeSpeed, Camera().StrafeSpeed, 1.0f, 1000.0f);

				return IsEdited;
			});

		RenderComponent<Light, false>(
			"Light",
			SelectedEntity,
			[&](Light& Component)
			{
				bool IsEdited = false;

				auto Type = (int*)&Component.Type;

				const char* LightTypes[] = { "Point", "Quad" };
				IsEdited |= ImGui::Combo("Type", Type, LightTypes, ARRAYSIZE(LightTypes), ARRAYSIZE(LightTypes));

				switch (Component.Type)
				{
				case ELightTypes::Point:
					IsEdited |= RenderFloat3Control("I", &Component.I.x);
					break;
				case ELightTypes::Quad:
					IsEdited |= RenderFloat3Control("I", &Component.I.x);
					IsEdited |= RenderFloatControl("Width", &Component.Width, 1.0f);
					IsEdited |= RenderFloatControl("Height", &Component.Height, 1.0f);
					break;
				}

				return IsEdited;
			});

		RenderComponent<MeshFilter, false>(
			"Mesh Filter",
			SelectedEntity,
			[&](MeshFilter& Component)
			{
				bool IsEdited = false;

				Component.Mesh = AssetManager::GetMeshCache().GetValidAsset(Component.Handle);

				ImGui::Text("Mesh: ");
				ImGui::SameLine();
				if (Component.Mesh)
				{
					ImGui::Button(Component.Mesh->Name.data());
				}
				else
				{
					ImGui::Button("<unknown>");
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ASSET_MESH"); Payload)
					{
						IM_ASSERT(Payload->DataSize == sizeof(AssetHandle));
						Component.Handle = *static_cast<AssetHandle*>(Payload->Data);

						IsEdited = true;
					}
					ImGui::EndDragDropTarget();
				}

				return IsEdited;
			});

		RenderComponent<MeshRenderer, false>(
			"Mesh Renderer",
			SelectedEntity,
			[&](MeshRenderer& Component)
			{
				bool IsEdited = false;

				if (ImGui::TreeNode("Material"))
				{
					auto& Material = Component.Material;

					ImGui::Text("Attributes");

					const char* BsdfTypes[(int)EBSDFTypes::NumBSDFTypes] = { "Lambertian",
																			 "Mirror",
																			 "Glass",
																			 "Disney" };
					IsEdited |= ImGui::Combo(
						"Type",
						reinterpret_cast<int*>(&Material.BSDFType),
						BsdfTypes,
						ARRAYSIZE(BsdfTypes),
						ARRAYSIZE(BsdfTypes));

					switch (Material.BSDFType)
					{
					case EBSDFTypes::Lambertian:
						[[fallthrough]];
					case EBSDFTypes::Mirror:
						IsEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						break;
					case EBSDFTypes::Glass:
						IsEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						IsEdited |= RenderFloat3Control("T", &Material.T.x);
						IsEdited |= ImGui::SliderFloat("etaA", &Material.etaA, 1, 3);
						IsEdited |= ImGui::SliderFloat("etaB", &Material.etaB, 1, 3);
						break;
					case EBSDFTypes::Disney:
						IsEdited |= RenderFloat3Control("Base Color", &Material.baseColor.x);
						IsEdited |= ImGui::SliderFloat("Metallic", &Material.metallic, 0, 1);
						IsEdited |= ImGui::SliderFloat("Subsurface", &Material.subsurface, 0, 1);
						IsEdited |= ImGui::SliderFloat("Specular", &Material.specular, 0, 1);
						IsEdited |= ImGui::SliderFloat("Roughness", &Material.roughness, 0, 1);
						IsEdited |= ImGui::SliderFloat("SpecularTint", &Material.specularTint, 0, 1);
						IsEdited |= ImGui::SliderFloat("Anisotropic", &Material.anisotropic, 0, 1);
						IsEdited |= ImGui::SliderFloat("Sheen", &Material.sheen, 0, 1);
						IsEdited |= ImGui::SliderFloat("SheenTint", &Material.sheenTint, 0, 1);
						IsEdited |= ImGui::SliderFloat("Clearcoat", &Material.clearcoat, 0, 1);
						IsEdited |= ImGui::SliderFloat("ClearcoatGloss", &Material.clearcoatGloss, 0, 1);
						break;
					default:
						break;
					}

					auto ImageBox = [&](ETextureTypes Type, MaterialTexture& MaterialTexture, std::string_view Name)
					{
						MaterialTexture.Texture = AssetManager::GetTextureCache().GetValidAsset(MaterialTexture.Handle);

						ImGui::Text(Name.data());
						ImGui::SameLine();
						if (MaterialTexture.Texture)
						{
							ImGui::Button(MaterialTexture.Texture->Name.data());
							Material.TextureIndices[Type] = MaterialTexture.Texture->SRV.GetIndex();
						}
						else
						{
							ImGui::Button("<unknown>");
							Material.TextureIndices[Type] = -1;
						}

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ASSET_IMAGE"); Payload)
							{
								IM_ASSERT(Payload->DataSize == sizeof(AssetHandle));
								MaterialTexture.Handle = *static_cast<AssetHandle*>(Payload->Data);

								IsEdited = true;
							}
							ImGui::EndDragDropTarget();
						}
					};

					ImageBox(ETextureTypes::AlbedoIdx, Material.Albedo, "Albedo: ");

					ImGui::TreePop();
				}

				return IsEdited;
			});

		RenderComponent<BoxCollider, false>(
			"Box Collider",
			SelectedEntity,
			[&](BoxCollider& Component)
			{
				bool IsEdited = false;

				IsEdited |= RenderFloat3Control("Extents", &Component.Extents.x);

				return IsEdited;
			});

		RenderComponent<CapsuleCollider, false>(
			"Capsule Collider",
			SelectedEntity,
			[&](CapsuleCollider& Component)
			{
				bool IsEdited = false;

				IsEdited |= RenderFloatControl("Radius", &Component.Radius);
				IsEdited |= RenderFloatControl("Height", &Component.Height);

				return IsEdited;
			});

		RenderComponent<MeshCollider, false>(
			"Mesh Collider",
			SelectedEntity,
			[&](MeshCollider& Component)
			{
				return false;
			});

		RenderComponent<StaticRigidBody, false>(
			"Static Rigid Body",
			SelectedEntity,
			[&](StaticRigidBody& Component)
			{
				bool IsEdited = false;

				return IsEdited;
			});

		RenderComponent<DynamicRigidBody, false>(
			"Dynamic Rigid Body",
			SelectedEntity,
			[&](DynamicRigidBody& Component)
			{
				bool IsEdited = false;

				return IsEdited;
			});

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("Component List");
		}

		if (ImGui::BeginPopup("Component List"))
		{
			AddNewComponent<MeshFilter>("Mesh Filter", SelectedEntity);
			AddNewComponent<MeshRenderer>("Mesh Renderer", SelectedEntity);
			AddNewComponent<Light>("Light", SelectedEntity);

			AddNewComponent<BoxCollider>("Box Collider", SelectedEntity);
			AddNewComponent<CapsuleCollider>("Capsule Collider", SelectedEntity);
			AddNewComponent<MeshCollider>("Mesh Collider", SelectedEntity);

			AddNewComponent<StaticRigidBody>("Static Rigid Body", SelectedEntity);
			AddNewComponent<DynamicRigidBody>("Dynamic Rigid Body", SelectedEntity);

			ImGui::EndPopup();
		}
	}
}
