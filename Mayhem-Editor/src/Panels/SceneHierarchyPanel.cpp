#include "SceneHierarchyPanel.h"

#include <glm/glm.hpp>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <cstring>
#include <cctype>

namespace Mayhem
{

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& _scene)
	{
		SetContext(_scene);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& _scene)
	{
		m_selectionContext = { entt::null, _scene.get() };
		m_Context = _scene;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Hierarchy");

		entt::iterable_adaptor each = m_Context->m_Registry.storage<entt::entity>().each();

		for (auto [entity] : each)
		{
			DrawEntityNode({ entity, m_Context.get() });
		}

		// Right - click blank space
		if (ImGui::BeginPopupContextWindow("Add entity", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
				m_Context->CreateEntity("Empty Entity");

			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Begin("Properties");
		if (m_selectionContext)
		{
			DrawComponents(m_selectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity _entity)
	{
		TagComponent tag = _entity.GetComponent<TagComponent>();

		ImGuiTreeNodeFlags selected = (m_selectionContext == _entity) ? ImGuiTreeNodeFlags_Selected : 0;
		ImGuiTreeNodeFlags flags = selected | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)_entity, flags, tag.Tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_selectionContext = _entity;
		}

		std::string label = "Remove Entity##" + std::to_string((uint32_t)_entity) + tag.Tag;
		bool deleted = false;
		if (ImGui::BeginPopupContextItem(label.c_str(), ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Delete entity"))
				deleted = true;

			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGui::TreePop();
		}

		if (deleted)
		{
			m_Context->DestroyEntity(_entity);
			m_selectionContext = Entity(entt::null, m_Context.get());
		}
	}

	static void DrawVec3Control(const std::string& _label, glm::vec3& values, float _reset = 0.0f, float _columnW = 100.f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto bold = io.Fonts->Fonts[0];

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, _columnW);
		ImGui::Text(_label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 ButtonSize = { lineHeight + 3.0f, lineHeight };


		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
		std::string label = "##X" + _label;
		std::string button = "X##" + _label;
		ImGui::PushFont(bold);
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.x = _reset;
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::DragFloat(label.c_str(), &values.x, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(3);
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.8f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.8f, 0.1f, 1.0f));
		label = "##Y" + _label;
		button = "Y##" + _label;
		ImGui::PushFont(bold);
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.y = _reset;
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::DragFloat(label.c_str(), &values.y, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(3);
		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.2f, 0.9f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.15f, 0.8f, 1.0f));
		label = "##Z" + _label;
		button = "Z##" + _label;
		ImGui::PushFont(bold);
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.z = _reset;
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::DragFloat(label.c_str(), &values.z, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();

		ImGui::Columns(1);
	}

	// =====================================================================
	//  Champs d'assets (texture, mesh, material…) façon Unity
	// =====================================================================

	enum class AssetFieldResult { None, Dropped, Cleared };

	// Remplace ImGui::SeparatorText (absent des versions d'ImGui antérieures à 1.89.4)
	static void SectionTitle(const char* _text)
	{
		ImGui::Spacing();
		ImGui::TextDisabled("%s", _text);
		ImGui::Separator();
	}

	static std::string ToLower(std::string _s)
	{
		for (auto& c : _s) c = (char)std::tolower((unsigned char)c);
		return _s;
	}

	// Champ en lecture seule + croix pour vider + drop filtré par extension.
	// _outPath reçoit le chemin déposé quand le résultat vaut Dropped.
	static AssetFieldResult DrawAssetField(const char* _label, const std::string& _displayName, bool _hasValue,
		std::initializer_list<const char*> _extensions, std::string& _outPath, const std::string& _tooltip = "")
	{
		AssetFieldResult result = AssetFieldResult::None;
		ImGui::PushID(_label);

		if (_hasValue)
		{
			if (ImGui::Button("X"))
				result = AssetFieldResult::Cleared;
			ImGui::SameLine();
		}

		char buffer[256];
		strncpy_s(buffer, sizeof(buffer), _displayName.c_str(), _TRUNCATE);
		ImGui::InputText(_label, buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

		if (!_tooltip.empty() && ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", _tooltip.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::filesystem::path path = (const wchar_t*)payload->Data;
				std::string ext = ToLower(path.extension().string());

				for (const char* allowed : _extensions)
				{
					if (ext == allowed)
					{
						_outPath = path.generic_string();
						result = AssetFieldResult::Dropped;
						break;
					}
				}
			}
			ImGui::EndDragDropTarget(); // toujours appelé, même sans drop
		}

		ImGui::PopID();
		return result;
	}

	// Crée assets/materials/NewMaterial(N).mat
	static Ref<Material> CreateNewMaterialAsset(bool _lit)
	{
		std::filesystem::path dir = "assets/materials";
		std::filesystem::create_directories(dir);

		std::filesystem::path path;
		int index = 0;
		do
		{
			path = dir / ("NewMaterial" + (index ? " " + std::to_string(index) : std::string()) + ".mat");
			index++;
		} while (std::filesystem::exists(path));

		Ref<Material> mat = Material::CreateAsset(path.generic_string());
		if (mat && !_lit)
		{
			mat->m_Lit = false;
			mat->Save();
		}
		return mat;
	}

	// Slot "Material" dans un component (Sprite / Mesh Renderer)
	static void DrawMaterialSlot(Ref<Material>& _material, std::string& _materialPath, bool _litByDefault)
	{
		// Chargement paresseux (scène désérialisée : seul le chemin est connu)
		if (!_material && !_materialPath.empty())
			_material = Material::Load(_materialPath);

		std::string name = _material
			? std::filesystem::path(_material->GetPath()).stem().string()
			: (_litByDefault ? "Default-Material" : "Default-Unlit");

		std::string dropped;
		switch (DrawAssetField("Material", name, _material != nullptr, { ".mat" }, dropped, _material ? _material->GetPath() : ""))
		{
		case AssetFieldResult::Dropped:
			if (Ref<Material> mat = Material::Load(dropped))
			{
				_material = mat;
				_materialPath = dropped;
			}
			break;
		case AssetFieldResult::Cleared:
			_material.reset();
			_materialPath.clear();
			break;
		default:
			break;
		}

		ImGui::SameLine();
		if (ImGui::Button("+##NewMaterial"))
		{
			if (Ref<Material> mat = CreateNewMaterialAsset(_litByDefault))
			{
				_material = mat;
				_materialPath = mat->GetPath();
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Creer un nouveau materiau dans assets/materials");
	}

	// Section matériau en bas de l'inspecteur (comme Unity). Sauvegarde automatique du .mat.
	static void DrawMaterialInspector(const Ref<Material>& _material, bool _litByDefault)
	{
		const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

		std::string name = _material
			? std::filesystem::path(_material->GetPath()).stem().string()
			: (_litByDefault ? "Default-Material" : "Default-Unlit");

		ImGui::Separator();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		bool open = ImGui::TreeNodeEx(("Material : " + name + "##MaterialInspector").c_str(), treeFlags);
		ImGui::PopStyleVar();

		if (!open)
			return;

		if (!_material)
		{
			ImGui::TextDisabled("Materiau par defaut (non modifiable).");
			ImGui::TextDisabled("Cree ou glisse un .mat dans le slot Material pour l'editer.");
			ImGui::TreePop();
			return;
		}

		Material& mat = *_material;
		bool save = false;

		// Sauvegarde quand l'utilisateur termine une modification (relâche le slider, valide…)
		auto saveIfEdited = [&save]() { if (ImGui::IsItemDeactivatedAfterEdit()) save = true; };

		ImGui::PushID("MaterialInspector");

		// ---------- Rendu ----------
		SectionTitle("Rendu");
		std::string dropped;
		std::string shaderName = std::filesystem::path(mat.m_ShaderPath).filename().string();
		switch (DrawAssetField("Shader", shaderName, !mat.m_ShaderPath.empty(), { ".glsl" }, dropped, mat.m_ShaderPath))
		{
		case AssetFieldResult::Dropped: mat.m_ShaderPath = dropped; save = true; break;
		case AssetFieldResult::Cleared: mat.m_ShaderPath = Material().m_ShaderPath; save = true; break; // revient au Standard
		default: break;
		}

		// ---------- Surface ----------
		SectionTitle("Surface");
		ImGui::ColorEdit4("Albedo", glm::value_ptr(mat.m_AlbedoColor));
		saveIfEdited();

		std::string albedoName = mat.m_AlbedoMapPath.empty() ? "None" : std::filesystem::path(mat.m_AlbedoMapPath).filename().string();
		switch (DrawAssetField("Albedo Map", albedoName, !mat.m_AlbedoMapPath.empty(), { ".png", ".jpg", ".jpeg", ".tga" }, dropped, mat.m_AlbedoMapPath))
		{
		case AssetFieldResult::Dropped: mat.m_AlbedoMapPath = dropped; save = true; break;
		case AssetFieldResult::Cleared: mat.m_AlbedoMapPath.clear(); save = true; break;
		default: break;
		}

		ImGui::DragFloat2("Tiling", glm::value_ptr(mat.m_Tiling), 0.05f);
		saveIfEdited();
		ImGui::DragFloat2("Offset", glm::value_ptr(mat.m_Offset), 0.01f);
		saveIfEdited();
		ImGui::SliderFloat("Alpha Cutoff", &mat.m_AlphaCutoff, 0.0f, 1.0f);
		saveIfEdited();

		// ---------- Eclairage ----------
		SectionTitle("Eclairage");
		if (ImGui::Checkbox("Lit", &mat.m_Lit))
			save = true;

		// Le spéculaire n'a de sens que si le matériau est éclairé
		if (mat.m_Lit)
		{
			ImGui::SliderFloat("Specular", &mat.m_SpecularStrength, 0.0f, 1.0f);
			saveIfEdited();
			ImGui::DragFloat("Shininess", &mat.m_Shininess, 1.0f, 1.0f, 512.0f);
			saveIfEdited();
		}

		ImGui::ColorEdit3("Emission", glm::value_ptr(mat.m_EmissionColor));
		saveIfEdited();
		ImGui::DragFloat("Emission Intensity", &mat.m_EmissionIntensity, 0.05f, 0.0f, 100.0f);
		saveIfEdited();

		ImGui::PopID();

		if (save)
			mat.Save();

		ImGui::TreePop();
	}

	// =====================================================================

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& _name, Entity _entity, UIFunction _func)
	{

		const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
		if (_entity.HasComponent<T>())
		{
			T& component = _entity.GetComponent<T>();
			ImVec2 contentAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();

			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeFlags, _name.c_str());
			ImGui::PopStyleVar();

			ImGui::SameLine(contentAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("...", ImVec2{ lineHeight,lineHeight }))
			{
				ImGui::OpenPopup("Component Settings");
			}

			bool remove = false;
			if (ImGui::BeginPopup("Component Settings"))
			{
				if (ImGui::MenuItem("Remove component"))
					remove = true;

				ImGui::EndPopup();
			}
			if (open)
			{
				_func(component);
				ImGui::TreePop();
			}

			if (remove)
				_entity.RemoveComponent<T>();
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity _entity)
	{
		if (_entity)
		{
			if (_entity.HasComponent<IDComponent>())
			{
				ImGui::Text(std::to_string((uint32_t)_entity.GetUUID()).c_str());
			}

			if (_entity.HasComponent<TagComponent>())
			{
				auto& tag = _entity.GetComponent<TagComponent>().Tag;

				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, sizeof(buffer), tag.c_str());
				std::string label = "##" + std::to_string((uint32_t)_entity);
				if (ImGui::InputText(label.c_str(), buffer, sizeof(char) * 256))
				{
					tag = std::string(buffer);
				}

				ImGui::SameLine();
				ImGui::PushItemWidth(-1);

				if (ImGui::Button("Add Component"))
					ImGui::OpenPopup("ComponentsList");

				if (ImGui::BeginPopup("ComponentsList"))
				{
					if (!_entity.HasComponent<CameraComponent>())
					{
						if (ImGui::MenuItem("Camera"))
						{
							m_selectionContext.AddComponent<CameraComponent>();
							ImGui::CloseCurrentPopup();
						}
					}
					if (!_entity.HasComponent<SpriteRenderer>())
					{
						if (ImGui::MenuItem("Sprite"))
						{
							m_selectionContext.AddComponent<SpriteRenderer>();
							ImGui::CloseCurrentPopup();
						}
					}
					if (!_entity.HasComponent<MeshRenderer>())
					{
						if (ImGui::MenuItem("Mesh"))
						{
							m_selectionContext.AddComponent<MeshRenderer>();
							ImGui::CloseCurrentPopup();
						}
					}
					if (!_entity.HasComponent<DirectionalLight>())
					{
						if (ImGui::MenuItem("Directional Light"))
						{
							m_selectionContext.AddComponent<DirectionalLight>();
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::EndPopup();
				}

				ImGui::PopItemWidth();
			}


			DrawComponent<TransformComponent>("Transform", _entity, [](TransformComponent& _component)
				{
					glm::vec3 rot = glm::degrees(_component.m_Rotation);
					DrawVec3Control("Position", _component.m_Translation);
					DrawVec3Control("Rotation", rot);
					DrawVec3Control("Scale", _component.m_Scale, 1.0f);
					_component.m_Rotation = glm::radians(rot);
				});

			DrawComponent<CameraComponent>("Camera", _entity, [](CameraComponent& _component)
				{
					bool primary = _component.m_Primary;
					if (ImGui::Checkbox("Primary", &primary))
						_component.m_Primary = primary;

					ImGui::Separator();

					const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
					const char* currentProjection = projectionTypeStrings[(int)_component.m_Camera.GetProjectionType()];
					if (ImGui::BeginCombo("Projection", currentProjection))
					{
						for (int i = 0; i < 2; i++)
						{
							bool isSelected = currentProjection == projectionTypeStrings[i];

							if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
							{
								currentProjection = projectionTypeStrings[i];
								_component.m_Camera.SetProjectionType((SceneCamera::ProjectionType)i);
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					if (_component.m_Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
					{
						float fov = glm::degrees(_component.m_Camera.GetFOV());
						float nearClip = _component.m_Camera.GetPerspectiveNearClip();
						float farClip = _component.m_Camera.GetPerspectiveFarClip();

						if (ImGui::DragFloat("Field of view", &fov, 1.f))
							_component.m_Camera.SetFOV(glm::radians(fov));
						if (ImGui::DragFloat("Near clip", &nearClip, 1.f))
							_component.m_Camera.SetPerspectiveNearClip(nearClip);
						if (ImGui::DragFloat("Far clip", &farClip, 10.f))
							_component.m_Camera.SetPerspectiveFarClip(farClip);
					}
					if (_component.m_Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
					{
						float zoom = _component.m_Camera.GetOrthographicSize();
						float nearClip = _component.m_Camera.GetOrthographicNearClip();
						float farClip = _component.m_Camera.GetOrthographicFarClip();

						if (ImGui::DragFloat("Zoom", &zoom, 0.25f, 1.0f, 50.f))
							_component.m_Camera.SetOrthographicSize(zoom);
						if (ImGui::DragFloat("Near clip", &nearClip, 1.f))
							_component.m_Camera.SetOrthographicNearClip(nearClip);
						if (ImGui::DragFloat("Far clip", &farClip, 10.f))
							_component.m_Camera.SetOrthographicFarClip(farClip);

						ImGui::Separator();

						if (ImGui::Checkbox("Fixed aspect ratio", &_component.m_FixedAspectRatio))
							_component.m_Camera.SetOrthographicSize(_component.m_Camera.GetOrthographicSize());
					}
				});

			DrawComponent<SpriteRenderer>("Sprite Renderer", _entity, [](SpriteRenderer& _component)
				{
					ImGui::ColorEdit4("Color", glm::value_ptr(_component.m_Color));

					std::string dropped;
					std::string textureName = _component.m_Texture ? _component.m_Texture->GetPath() : "None";
					switch (DrawAssetField("Texture", textureName, _component.m_Texture != nullptr, { ".png", ".jpg", ".jpeg", ".tga" }, dropped))
					{
					case AssetFieldResult::Dropped: _component.m_Texture = Texture2D::Create(dropped); break;
					case AssetFieldResult::Cleared: _component.m_Texture.reset(); break;
					default: break;
					}

					DrawMaterialSlot(_component.m_Material, _component.m_MaterialPath, false);
				});

			DrawComponent<MeshRenderer>("Mesh Renderer", _entity, [](MeshRenderer& _component)
				{
					ImGui::ColorEdit4("Color", glm::value_ptr(_component.m_Color));

					std::string dropped;
					std::string meshName = _component.m_MeshPath.empty() ? "None" : std::filesystem::path(_component.m_MeshPath).filename().string();
					switch (DrawAssetField("Mesh", meshName, !_component.m_MeshPath.empty(), { ".fbx", ".obj", ".gltf", ".glb" }, dropped, _component.m_MeshPath))
					{
					case AssetFieldResult::Dropped:
						_component.SetMesh(dropped); // garde aussi le chemin pour la sérialisation
						break;
					case AssetFieldResult::Cleared:
						_component.m_Mesh.reset();
						_component.m_MeshPath.clear();
						break;
					default:
						break;
					}

					DrawMaterialSlot(_component.m_Material, _component.m_MaterialPath, true);
				});

			DrawComponent<DirectionalLight>("Directional Light", _entity, [](DirectionalLight& _component)
				{
					ImGui::ColorEdit3("Color", glm::value_ptr(_component.m_Color));

					ImGui::DragFloat("Intensity", &_component.m_Intensity, 0.05f, 0.0f, 100.f);
					ImGui::DragFloat("Ambient", &_component.m_Ambient, 0.01f, 0.0f, 1.f);
				});

			// ---------- Matériaux en bas de l'inspecteur, comme Unity ----------
			if (_entity.HasComponent<MeshRenderer>())
			{
				ImGui::PushID("MeshMaterial");
				DrawMaterialInspector(_entity.GetComponent<MeshRenderer>().m_Material, true);
				ImGui::PopID();
			}

			if (_entity.HasComponent<SpriteRenderer>())
			{
				ImGui::PushID("SpriteMaterial");
				DrawMaterialInspector(_entity.GetComponent<SpriteRenderer>().m_Material, false);
				ImGui::PopID();
			}
		}
	}

}
