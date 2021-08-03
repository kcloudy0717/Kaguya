#include "InspectorWindow.h"

#include <imgui_internal.h>

#include <Graphics/AssetManager.h>

template<class T, class Component>
concept IsUIFunction = requires(T F, Component C)
{
	{
		F(C)
		} -> std::convertible_to<bool>;
};

template<is_component T, bool IsCoreComponent, IsUIFunction<T> UIFunction>
static void RenderComponent(const char* pName, Entity Entity, UIFunction UI)
{
	if (Entity.HasComponent<T>())
	{
		bool  IsEdited	= false;
		auto& Component = Entity.GetComponent<T>();

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		const ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
												 ImGuiTreeNodeFlags_SpanAvailWidth |
												 ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::Separator();
		bool Collapsed = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), TreeNodeFlags, pName);
		ImGui::PopStyleVar();

		ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
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
			IsEdited |= UI(Component);
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

template<is_component T>
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

static bool RenderButtonDragFloatControl(
	std::string_view ButtonLabel,
	std::string_view DragFloatLabel,
	float*			 pFloat,
	float			 ResetValue,
	float			 Min,
	float			 Max,
	ImVec4			 ButtonColor,
	ImVec4			 ButtonHoveredColor,
	ImVec4			 ButtonActiveColor)
{
	bool isEdited = false;

	ImGuiIO& IO		  = ImGui::GetIO();
	auto	 boldFont = IO.Fonts->Fonts[0];

	const float	 lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	const ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ButtonColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActiveColor);
	ImGui::PushFont(boldFont);
	if (ImGui::Button(ButtonLabel.data(), buttonSize))
	{
		*pFloat = ResetValue;
		isEdited |= true;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	isEdited |= ImGui::DragFloat(DragFloatLabel.data(), pFloat, 0.1f, Min, Max);
	ImGui::PopItemWidth();

	return isEdited;
}

static bool RenderFloatControl(
	std::string_view Label,
	float*			 pFloat,
	float			 ResetValue = 0.0f,
	float			 Min		= 0.0f,
	float			 Max		= 0.0f)
{
	bool isEdited = false;
	if (ImGui::BeginTable("Float", 2, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(1, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		isEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&pFloat[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return isEdited;
}

static bool RenderFloat2Control(
	std::string_view Label,
	float*			 pFloat2,
	float			 ResetValue = 0.0f,
	float			 Min		= 0.0f,
	float			 Max		= 0.0f)
{
	bool isEdited = false;
	if (ImGui::BeginTable("Float2", 3, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		isEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&pFloat2[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(2);
		isEdited |= RenderButtonDragFloatControl(
			"Y",
			"##Y",
			&pFloat2[1],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.9f, 0.1f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return isEdited;
}

static bool RenderFloat3Control(
	std::string_view Label,
	float*			 pFloat3,
	float			 ResetValue = 0.0f,
	float			 Min		= 0.0f,
	float			 Max		= 0.0f)
{
	bool isEdited = false;
	if (ImGui::BeginTable("Float3", 4, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		isEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&pFloat3[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(2);
		isEdited |= RenderButtonDragFloatControl(
			"Y",
			"##Y",
			&pFloat3[1],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.9f, 0.1f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(3);
		isEdited |= RenderButtonDragFloatControl(
			"Z",
			"##Z",
			&pFloat3[2],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.1f, 0.9f, 1.0f },
			ImVec4{ 0.2f, 0.2f, 1.0f, 1.0f },
			ImVec4{ 0.2f, 0.2f, 1.0f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return isEdited;
}

static bool EditTransform(Transform& Transform, const float* pCameraView, float* pCameraProjection, float* pMatrix)
{
	static bool			  UseSnap				= false;
	static UINT			  CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	static float		  s_Snap[3]				= { 1, 1, 1 };
	static ImGuizmo::MODE s_CurrentGizmoMode	= ImGuizmo::WORLD;

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
		ImGui::InputFloat3("Snap", &s_Snap[0]);
		break;
	case ImGuizmo::ROTATE:
		ImGui::InputFloat("Angle Snap", &s_Snap[0]);
		break;
	case ImGuizmo::SCALE:
		ImGui::InputFloat("Scale Snap", &s_Snap[0]);
		break;
	}

	return ImGuizmo::Manipulate(
		pCameraView,
		pCameraProjection,
		(ImGuizmo::OPERATION)CurrentGizmoOperation,
		s_CurrentGizmoMode,
		pMatrix,
		nullptr,
		UseSnap ? s_Snap : nullptr);
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
				std::memcpy(Buffer, Component.Name.data(), Component.Name.size());
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

				float matrixTranslation[3], matrixRotation[3], matrixScale[3];
				ImGuizmo::DecomposeMatrixToComponents(
					reinterpret_cast<float*>(&World),
					matrixTranslation,
					matrixRotation,
					matrixScale);
				IsEdited |= RenderFloat3Control("Translation", matrixTranslation);
				IsEdited |= RenderFloat3Control("Rotation", matrixRotation);
				IsEdited |= RenderFloat3Control("Scale", matrixScale, 1.0f);
				ImGuizmo::RecomposeMatrixFromComponents(
					matrixTranslation,
					matrixRotation,
					matrixScale,
					reinterpret_cast<float*>(&World));

				// Dont transpose this
				XMStoreFloat4x4(&View, pWorld->ActiveCamera->ViewMatrix);
				XMStoreFloat4x4(&Projection, pWorld->ActiveCamera->ProjectionMatrix);

				// If we have edited the transform, update it and mark it as dirty so it will be updated on the GPU side
				IsEdited |= EditTransform(
					Component,
					reinterpret_cast<float*>(&View),
					reinterpret_cast<float*>(&Projection),
					reinterpret_cast<float*>(&World));

				if (IsEdited)
				{
					DirectX::XMMATRIX mWorld = XMLoadFloat4x4(&World);
					Component.SetTransform(mWorld);
				}

				return IsEdited;
			});

		RenderComponent<MeshFilter, false>(
			"Mesh Filter",
			SelectedEntity,
			[&](MeshFilter& Component)
			{
				bool IsEdited = false;

				auto handle = AssetManager::GetMeshCache().Load(Component.Key);

				ImGui::Text("Mesh: ");
				ImGui::SameLine();
				if (handle)
				{
					ImGui::Button(handle->Name.data());
				}
				else
				{
					ImGui::Button("NULL");
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MESH"); payload)
					{
						IM_ASSERT(payload->DataSize == sizeof(UINT64));
						Component.Key  = (*(UINT64*)payload->Data);
						Component.Mesh = AssetManager::GetMeshCache().Load(Component.Key);

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

					const char* BSDFTypes[(int)EBSDFTypes::NumBSDFTypes] = { "Lambertian",
																			 "Mirror",
																			 "Glass",
																			 "Disney" };
					IsEdited |= ImGui::Combo(
						"Type",
						(int*)&Material.BSDFType,
						BSDFTypes,
						ARRAYSIZE(BSDFTypes),
						ARRAYSIZE(BSDFTypes));

					switch (Material.BSDFType)
					{
					case EBSDFTypes::Lambertian:
						IsEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						break;
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
					}

					auto ImageBox = [&](ETextureTypes Type, UINT64& Key, std::string_view Name)
					{
						auto handle = AssetManager::GetImageCache().Load(Key);

						ImGui::Text(Name.data());
						ImGui::SameLine();
						if (handle)
						{
							ImGui::Button(handle->Name.data());
							Material.TextureIndices[Type] = handle->SRV.GetIndex();
						}
						else
						{
							ImGui::Button("NULL");
							Material.TextureIndices[Type] = -1;
						}

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_IMAGE"); payload)
							{
								IM_ASSERT(payload->DataSize == sizeof(UINT64));
								Key = (*(UINT64*)payload->Data);

								IsEdited = true;
							}
							ImGui::EndDragDropTarget();
						}
					};

					ImageBox(ETextureTypes::AlbedoIdx, Material.Albedo.Key, "Albedo: ");

					ImGui::TreePop();
				}

				return IsEdited;
			});

		RenderComponent<Light, false>(
			"Light",
			SelectedEntity,
			[&](Light& Component)
			{
				bool IsEdited = false;

				auto pType = (int*)&Component.Type;

				const char* LightTypes[] = { "Point", "Quad" };
				IsEdited |= ImGui::Combo("Type", pType, LightTypes, ARRAYSIZE(LightTypes), ARRAYSIZE(LightTypes));

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
