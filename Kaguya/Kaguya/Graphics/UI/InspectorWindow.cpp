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
static void RenderComponent(const char* pName, Entity Entity, UIFunction UI, bool (*UISettingFunction)(T&))
{
	if (Entity.HasComponent<T>())
	{
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

			if (UISettingFunction)
			{
				Component.IsEdited |= UISettingFunction(Component);
			}

			ImGui::EndPopup();
		}

		if (Collapsed)
		{
			Component.IsEdited |= UI(Component);
			ImGui::TreePop();
		}

		if (Component.IsEdited)
		{
			Component.IsEdited = false;

			Entity.pScene->SceneState |= ESceneState::SceneState_Update;
		}

		if (RemoveComponent)
		{
			Entity.RemoveComponent<T>();

			Entity.pScene->SceneState |= ESceneState::SceneState_Update;
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
	static float		  s_Snap[3]			 = { 1, 1, 1 };
	static ImGuizmo::MODE s_CurrentGizmoMode = ImGuizmo::WORLD;

	bool isEdited = false;

	ImGui::Text("Operation");

	if (ImGui::RadioButton("Translate", Transform.CurrentGizmoOperation == ImGuizmo::TRANSLATE))
	{
		Transform.CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", Transform.CurrentGizmoOperation == ImGuizmo::ROTATE))
	{
		Transform.CurrentGizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", Transform.CurrentGizmoOperation == ImGuizmo::SCALE))
	{
		Transform.CurrentGizmoOperation = ImGuizmo::SCALE;
	}

	ImGui::Checkbox("Snap", &Transform.UseSnap);
	ImGui::SameLine();

	switch (Transform.CurrentGizmoOperation)
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

	isEdited |= ImGuizmo::Manipulate(
		pCameraView,
		pCameraProjection,
		(ImGuizmo::OPERATION)Transform.CurrentGizmoOperation,
		s_CurrentGizmoMode,
		pMatrix,
		nullptr,
		Transform.UseSnap ? s_Snap : nullptr);

	return isEdited;
}

void InspectorWindow::RenderGui()
{
	ImGui::Begin("Inspector");

	UIWindow::Update();

	if (m_SelectedEntity)
	{
		RenderComponent<Tag, true>(
			"Tag",
			m_SelectedEntity,
			[](Tag& Component)
			{
				char buffer[MAX_PATH] = {};
				memcpy(buffer, Component.Name.data(), Component.Name.size());
				if (ImGui::InputText("Tag", buffer, MAX_PATH))
				{
					Component.Name = buffer;
				}

				return false;
			},
			nullptr);

		RenderComponent<Transform, true>(
			"Transform",
			m_SelectedEntity,
			[&](Transform& Component)
			{
				bool isEdited = false;

				DirectX::XMFLOAT4X4 world, view, projection;

				// Dont transpose this
				XMStoreFloat4x4(&world, Component.Matrix());

				float matrixTranslation[3], matrixRotation[3], matrixScale[3];
				ImGuizmo::DecomposeMatrixToComponents(
					reinterpret_cast<float*>(&world),
					matrixTranslation,
					matrixRotation,
					matrixScale);
				isEdited |= RenderFloat3Control("Translation", matrixTranslation);
				isEdited |= RenderFloat3Control("Rotation", matrixRotation);
				isEdited |= RenderFloat3Control("Scale", matrixScale, 1.0f);
				ImGuizmo::RecomposeMatrixFromComponents(
					matrixTranslation,
					matrixRotation,
					matrixScale,
					reinterpret_cast<float*>(&world));

				// Dont transpose this
				XMStoreFloat4x4(&view, m_pScene->Camera.ViewMatrix);
				XMStoreFloat4x4(&projection, m_pScene->Camera.ProjectionMatrix);

				// If we have edited the transform, update it and mark it as dirty so it will be updated on the GPU side
				isEdited |= EditTransform(
					Component,
					reinterpret_cast<float*>(&view),
					reinterpret_cast<float*>(&projection),
					reinterpret_cast<float*>(&world));

				if (isEdited)
				{
					DirectX::XMMATRIX mWorld = XMLoadFloat4x4(&world);
					Component.SetTransform(mWorld);
				}

				return isEdited;
			},
			[](auto Component)
			{
				bool isEdited = false;

				if (isEdited |= ImGui::MenuItem("Reset"))
				{
					Component = Transform();
				}

				return isEdited;
			});

		RenderComponent<MeshFilter, false>(
			"Mesh Filter",
			m_SelectedEntity,
			[&](MeshFilter& Component)
			{
				bool isEdited = false;

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

						isEdited = true;
					}
					ImGui::EndDragDropTarget();
				}

				return isEdited;
			},
			nullptr);

		RenderComponent<MeshRenderer, false>(
			"Mesh Renderer",
			m_SelectedEntity,
			[](MeshRenderer& Component)
			{
				bool isEdited = false;

				if (ImGui::TreeNode("Material"))
				{
					auto& Material = Component.Material;

					ImGui::Text("Attributes");

					const char* BSDFTypes[BSDFTypes::NumBSDFTypes] = { "Lambertian", "Mirror", "Glass", "Disney" };
					isEdited |=
						ImGui::Combo("Type", &Material.BSDFType, BSDFTypes, ARRAYSIZE(BSDFTypes), ARRAYSIZE(BSDFTypes));

					switch (Material.BSDFType)
					{
					case BSDFTypes::Lambertian:
						isEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						break;
					case BSDFTypes::Mirror:
						isEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						break;
					case BSDFTypes::Glass:
						isEdited |= RenderFloat3Control("R", &Material.baseColor.x);
						isEdited |= RenderFloat3Control("T", &Material.T.x);
						isEdited |= ImGui::SliderFloat("etaA", &Material.etaA, 1, 3);
						isEdited |= ImGui::SliderFloat("etaB", &Material.etaB, 1, 3);
						break;
					case BSDFTypes::Disney:
						isEdited |= RenderFloat3Control("Base Color", &Material.baseColor.x);
						isEdited |= ImGui::SliderFloat("Metallic", &Material.metallic, 0, 1);
						isEdited |= ImGui::SliderFloat("Subsurface", &Material.subsurface, 0, 1);
						isEdited |= ImGui::SliderFloat("Specular", &Material.specular, 0, 1);
						isEdited |= ImGui::SliderFloat("Roughness", &Material.roughness, 0, 1);
						isEdited |= ImGui::SliderFloat("SpecularTint", &Material.specularTint, 0, 1);
						isEdited |= ImGui::SliderFloat("Anisotropic", &Material.anisotropic, 0, 1);
						isEdited |= ImGui::SliderFloat("Sheen", &Material.sheen, 0, 1);
						isEdited |= ImGui::SliderFloat("SheenTint", &Material.sheenTint, 0, 1);
						isEdited |= ImGui::SliderFloat("Clearcoat", &Material.clearcoat, 0, 1);
						isEdited |= ImGui::SliderFloat("ClearcoatGloss", &Material.clearcoatGloss, 0, 1);
						break;
					default:
						break;
					}

					auto ImageBox = [&](TextureTypes TextureType, UINT64& Key, std::string_view Name)
					{
						auto handle = AssetManager::GetImageCache().Load(Key);

						ImGui::Text(Name.data());
						ImGui::SameLine();
						if (handle)
						{
							ImGui::Button(handle->Name.data());
							Material.TextureIndices[TextureType] = handle->SRV.GetIndex();
						}
						else
						{
							ImGui::Button("NULL");
							Material.TextureIndices[TextureType] = -1;
						}

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_IMAGE"); payload)
							{
								IM_ASSERT(payload->DataSize == sizeof(UINT64));
								Key = (*(UINT64*)payload->Data);

								isEdited = true;
							}
							ImGui::EndDragDropTarget();
						}
					};

					ImageBox(TextureTypes::AlbedoIdx, Material.TextureKeys[0], "Albedo: ");

					ImGui::TreePop();
				}

				return isEdited;
			},
			nullptr);

		RenderComponent<Light, false>(
			"Light",
			m_SelectedEntity,
			[](Light& Component)
			{
				bool isEdited = false;

				auto pType = (int*)&Component.Type;

				const char* LightTypes[] = { "Point", "Quad" };
				isEdited |= ImGui::Combo("Type", pType, LightTypes, ARRAYSIZE(LightTypes), ARRAYSIZE(LightTypes));

				switch (Component.Type)
				{
				case Light::Point:
					isEdited |= RenderFloat3Control("I", &Component.I.x);
					break;
				case Light::Quad:
					isEdited |= RenderFloat3Control("I", &Component.I.x);
					isEdited |= RenderFloatControl("Width", &Component.Width, 1.0f);
					isEdited |= RenderFloatControl("Height", &Component.Height, 1.0f);
					break;
				default:
					break;
				}

				return isEdited;
			},
			nullptr);

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("Component List");
		}

		if (ImGui::BeginPopup("Component List"))
		{
			AddNewComponent<MeshFilter>("Mesh Filter", m_SelectedEntity);
			AddNewComponent<MeshRenderer>("Mesh Renderer", m_SelectedEntity);
			AddNewComponent<Light>("Light", m_SelectedEntity);

			ImGui::EndPopup();
		}
	}
	ImGui::End();
}
