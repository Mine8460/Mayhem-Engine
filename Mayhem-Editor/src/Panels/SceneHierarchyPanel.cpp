#include "SceneHierarchyPanel.h"

#include <glm/glm.hpp>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <cctype>
#include <algorithm>
#include "ContentBrowserPanel.h"
#include <Engine/Scene/Prefab.h>
#include "../EditorWidgets.h"

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

	void SceneHierarchyPanel::SetSelectedEntity(Entity _entity)
	{
		m_selectionContext = _entity;

		// Sélectionner une entité désélectionne l'asset : l'inspecteur montre l'entité
		if (_entity && m_ContentBrowser)
			m_ContentBrowser->ClearSelection();
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		// ==================== Hierarchy ====================
		ImGui::Begin("Hierarchy");

		m_Pending = PendingActions();

		// Seulement les racines : chaque entité dessine ses enfants
		for (auto [entity] : m_Context->m_Registry.storage<entt::entity>().each())
		{
			Entity e{ entity, m_Context.get() };
			if (!e.GetParent())
				DrawEntityNode(e);
		}

		if (m_HasPendingPrefab)
		{
			Entity parent = m_Context->GetEntityByUUID(m_PendingPrefabParent);
			Entity instance = Prefab::Instantiate(*m_Context, m_PendingPrefabPath.lexically_normal().generic_string(), parent);
			if (instance)
				SetSelectedEntity(instance);
			m_HasPendingPrefab = false;
		}

		// Lâcher une entité dans le vide = la détacher de son parent
		if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->InnerRect, ImGui::GetID("##HierarchyRoot")))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
			{
				m_Pending.Reparent = true;
				m_Pending.ReparentChild = *(const uint64_t*)payload->Data;
				m_Pending.ReparentParent = 0;
			}
			ImGui::EndDragDropTarget();
			// Clic droit dans le vide
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::filesystem::path path = (const wchar_t*)payload->Data;
				if (path.extension() == Prefab::Extension)
				{
					m_PendingPrefabPath = path;
					m_PendingPrefabParent = 0;
					m_HasPendingPrefab = true;
				}
			}
		}


		if (ImGui::BeginPopupContextWindow("Add entity", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
				SetSelectedEntity(m_Context->CreateEntity("Empty Entity"));

			ImGui::EndPopup();
		}

		// ---------- Application des actions (après le parcours) ----------
		if (m_Pending.Reparent)
		{
			Entity child = m_Context->GetEntityByUUID(m_Pending.ReparentChild);
			Entity parent = m_Context->GetEntityByUUID(m_Pending.ReparentParent);
			m_Context->SetParent(child, parent); // refusé automatiquement si ça crée une boucle
		}

		if (m_Pending.CreateChild)
		{
			Entity parent = m_Context->GetEntityByUUID(m_Pending.CreateChildOf);
			Entity child = m_Context->CreateEntity("Empty Entity");
			child.SetParent(parent, false); // position locale (0,0,0) = au centre du parent
			SetSelectedEntity(child);
		}

		if (m_Pending.Delete)
		{
			if (m_selectionContext && (m_selectionContext == m_Pending.Delete || m_selectionContext.IsDescendantOf(m_Pending.Delete)))
				m_selectionContext = Entity(entt::null, m_Context.get());

			m_Context->DestroyEntity(m_Pending.Delete); // détruit aussi les enfants
		}

		ImGui::End();

		// ==================== Properties ====================
		ImGui::Begin("Properties");

		if (m_ContentBrowser && m_ContentBrowser->HasSelection())
		{
			// Asset sélectionné dans le Content Browser
			m_ContentBrowser->DrawSelectionInspector();
		}
		else if (m_selectionContext)
		{
			// Entité sélectionnée : bandeau prefab PUIS components
			Entity current = m_selectionContext;
			Entity next = EditorUI::PrefabInstanceBar(current, *m_Context, m_ContentBrowser);
			if (next != current)
				SetSelectedEntity(next);

			if (m_selectionContext)
				DrawComponents(m_selectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity _entity)
	{
		const std::string& tag = _entity.GetComponent<TagComponent>().Tag;
		const uint64_t uuid = (uint64_t)_entity.GetUUID();
		std::vector<Entity> children = _entity.GetChildren();

		ImGuiTreeNodeFlags flags = ((m_selectionContext == _entity) ? ImGuiTreeNodeFlags_Selected : 0)
			| ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf; // pas de flèche si pas d'enfant

		const bool inPrefab = (bool)Prefab::FindInstanceRoot(_entity);
		if (inPrefab)
		{
			const glm::vec4 c = EditorUI::GetPrefabColor();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(c.r, c.g, c.b, c.a));
		}

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)uuid, flags, "%s", tag.c_str());

		if (inPrefab)
			ImGui::PopStyleColor();
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			m_selectionContext = _entity;

		// Glisser cette entité...
		if (ImGui::BeginDragDropSource())
		{
			uint64_t uuid = (uint64_t)_entity.GetUUID();
			ImGui::SetDragDropPayload("SCENE_ENTITY", &uuid, sizeof(uint64_t));
			ImGui::Text("%s", _entity.GetComponent<TagComponent>().Tag.c_str());
			ImGui::EndDragDropSource();
		}

		// ...et la lâcher sur une autre pour en faire son enfant
		if (ImGui::BeginDragDropTarget())
		{
			// Parentage : même payload que la source ci-dessus
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
			{
				Entity dropped = m_Context->GetEntityByUUID(*(const uint64_t*)payload->Data);
				m_Context->SetParent(dropped, _entity); // garde ton code de parentage actuel ici
			}

			// Prefab lâché sur l'entité = instancié comme enfant
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::filesystem::path path = (const wchar_t*)payload->Data;
				if (path.extension() == Prefab::Extension)
				{
					m_PendingPrefabPath = path;
					m_PendingPrefabParent = (uint64_t)_entity.GetUUID();
					m_HasPendingPrefab = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Child Entity"))
			{
				m_Pending.CreateChild = true;
				m_Pending.CreateChildOf = uuid;
			}
			if (_entity.GetParent() && ImGui::MenuItem("Unparent"))
			{
				m_Pending.Reparent = true;
				m_Pending.ReparentChild = uuid;
				m_Pending.ReparentParent = 0;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Delete entity"))
				m_Pending.Delete = _entity;

			ImGui::EndPopup();
		}

		if (opened)
		{
			for (Entity child : children)
				DrawEntityNode(child);
			ImGui::TreePop();
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
	static void DrawMaterialSlot(const char* _label, Ref<Material>& _material, std::string& _materialPath, bool _litByDefault)
	{
		// Chargement paresseux (scène désérialisée : seul le chemin est connu)
		if (!_material && !_materialPath.empty())
			_material = Material::Load(_materialPath);

		ImGui::PushID(_label);

		std::string name = _material
			? std::filesystem::path(_material->GetPath()).stem().string()
			: (_litByDefault ? "Default-Material" : "Default-Unlit");

		std::string dropped;
		switch (EditorUI::AssetField(_label, name, _material != nullptr, { ".mat" }, dropped, _material ? _material->GetPath() : ""))
		{
		case EditorUI::AssetFieldResult::Dropped:
			if (Ref<Material> mat = Material::Load(dropped))
			{
				_material = mat;
				_materialPath = dropped;
			}
			break;
		case EditorUI::AssetFieldResult::Cleared:
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

		ImGui::PopID();
	}

	// Section matériau en bas de l'inspecteur (comme Unity). Sauvegarde automatique du .mat.
	// _slotLabel : nom du slot affiché dans l'en-tête quand le mesh a plusieurs matériaux
	static void DrawMaterialInspector(const Ref<Material>& _material, bool _litByDefault, const std::string& _slotLabel = "")
	{
		const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

		std::string name = _material
			? std::filesystem::path(_material->GetPath()).stem().string()
			: (_litByDefault ? "Default-Material" : "Default-Unlit");
		if (!_slotLabel.empty())
			name += "  (" + _slotLabel + ")";

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

		EditorUI::MaterialEditor(*_material);

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
					if (!_entity.HasComponent<PointLight>())
					{
						if (ImGui::MenuItem("Point Light"))
						{
							m_selectionContext.AddComponent<PointLight>();
							ImGui::CloseCurrentPopup();
						}
					}
					if (!_entity.HasComponent<SpotLight>())
					{
						if (ImGui::MenuItem("Spot Light"))
						{
							m_selectionContext.AddComponent<SpotLight>();
							ImGui::CloseCurrentPopup();
						}
					}
					if (!_entity.HasComponent<Skybox>())
					{
						if (ImGui::MenuItem("Skybox"))
						{
							m_selectionContext.AddComponent<Skybox>();
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

					DrawMaterialSlot("Material", _component.m_Material, _component.m_MaterialPath, false);
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

					// ---------- Slots de matériaux (un par matériau du fichier 3D) ----------
					if (!_component.m_Mesh && !_component.m_MeshPath.empty())
						_component.SetMesh(_component.m_MeshPath);

					const uint32_t slotCount = _component.GetMaterialSlotCount();
					if (_component.m_Materials.size() < slotCount)
						_component.m_Materials.resize(slotCount);

					SectionTitle(slotCount > 1 ? ("Materials (" + std::to_string(slotCount) + ")").c_str() : "Material");
					for (uint32_t i = 0; i < slotCount; i++)
					{
						MeshRenderer::MaterialSlot& slot = _component.m_Materials[i];
						const std::string slotName = _component.m_Mesh ? _component.m_Mesh->GetMaterialSlotName(i) : "Material";
						const std::string label = slotCount > 1 ? std::to_string(i) + " : " + slotName : std::string("Material");

						ImGui::PushID((int)i);
						DrawMaterialSlot(label.c_str(), slot.Asset, slot.Path, true);
						ImGui::PopID();
					}

					// Slots en trop (le mesh a changé et a moins de matériaux qu'avant)
					if (_component.m_Mesh && _component.m_Materials.size() > slotCount)
					{
						ImGui::TextDisabled("%zu slot(s) inutilise(s)", _component.m_Materials.size() - slotCount);
						ImGui::SameLine();
						if (ImGui::SmallButton("Retirer"))
							_component.m_Materials.resize(slotCount);
					}
				});

			DrawComponent<DirectionalLight>("Directional Light", _entity, [](DirectionalLight& _component)
				{
					ImGui::ColorEdit3("Color", glm::value_ptr(_component.m_Color));

					ImGui::DragFloat("Intensity", &_component.m_Intensity, 0.05f, 0.0f, 100.f);
					ImGui::DragFloat("Ambient", &_component.m_Ambient, 0.01f, 0.0f, 1.f);

					ImGui::Checkbox("Cast Shadows", &_component.m_CastShadows);
					if (_component.m_CastShadows)
					{
						ImGui::DragFloat("Shadow Distance", &_component.m_ShadowDistance, 0.5, 1.0f, 500.f);
						ImGui::DragFloat("Shadow Bias", &_component.m_ShadowBias, 0.00001, 0.0f, 0.05f, "%.4f");
						SectionTitle("Debug");
						ImGui::Checkbox("Show Cascades", &_component.m_ShowCascades);
					}
				});

			DrawComponent<PointLight>("Point Light", _entity, [](PointLight& _component)
				{
					ImGui::ColorEdit3("Color", glm::value_ptr(_component.m_Color));
					ImGui::DragFloat("Intensity", &_component.m_Intensity, 0.1f, 0.0f, 1000.f);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("1 = autant qu'un soleil d'intensite 1 a 1 m. Decroit avec le carre de la distance.");
					ImGui::DragFloat("Range", &_component.m_Range, 0.1f, 0.1f, 1000.f, "%.1f m");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Distance ou la lumiere s'eteint completement");

					ImGui::Checkbox("Cast Shadows", &_component.m_CastShadows);
					if (_component.m_CastShadows)
					{
						ImGui::DragFloat("Shadow Bias", &_component.m_ShadowBias, 0.002f, 0.0f, 1.0f, "%.3f m");
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("Monte si des rayures apparaissent sur les surfaces eclairees, baisse si l'ombre se decolle de l'objet");
					}
				});

			DrawComponent<SpotLight>("Spot Light", _entity, [](SpotLight& _component)
				{
					ImGui::ColorEdit3("Color", glm::value_ptr(_component.m_Color));
					ImGui::DragFloat("Intensity", &_component.m_Intensity, 0.1f, 0.0f, 1000.f);
					ImGui::DragFloat("Range", &_component.m_Range, 0.1f, 0.1f, 1000.f, "%.1f m");
					if (ImGui::SliderFloat("Spot Angle", &_component.m_OuterAngle, 1.0f, 179.0f, "%.0f deg"))
						_component.m_InnerAngle = glm::min(_component.m_InnerAngle, _component.m_OuterAngle);
					if (ImGui::SliderFloat("Inner Spot Angle", &_component.m_InnerAngle, 0.0f, _component.m_OuterAngle, "%.0f deg"))
						_component.m_InnerAngle = glm::min(_component.m_InnerAngle, _component.m_OuterAngle);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Zone en pleine lumiere : au-dela, fondu jusqu'au bord du cone");

					ImGui::Checkbox("Cast Shadows", &_component.m_CastShadows);
					if (_component.m_CastShadows)
					{
						ImGui::DragFloat("Shadow Bias", &_component.m_ShadowBias, 0.002f, 0.0f, 1.0f, "%.3f m");
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("Monte si des rayures apparaissent sur les surfaces eclairees, baisse si l'ombre se decolle de l'objet");
					}
				});

			DrawComponent<Skybox>("Skybox", _entity, [](Skybox& _component)
				{
					ImGui::ColorEdit3("Zenith", glm::value_ptr(_component.m_ZenithColor));
					ImGui::ColorEdit3("Horizon", glm::value_ptr(_component.m_HorizonColor));
					ImGui::ColorEdit3("Ground", glm::value_ptr(_component.m_GroundColor));
					ImGui::DragFloat("Sun Size", &_component.m_SunSize, 0.05f, 0.1f, 20.0f);
					ImGui::DragFloat("Sun Glow", &_component.m_SunGlow, 0.01f, 0.0f, 5.0f);
					ImGui::DragFloat("Exposure", &_component.m_Exposure, 0.01f, 0.0f, 5.0f);
					ImGui::DragFloat("Environment Intensity", &_component.m_EnvironmentIntensity, 0.01f, 0.0f, 5.0f);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Force de l'eclairage ambiant et des reflets venant du ciel (IBL)");
					SectionTitle("Gamma");
					ImGui::Checkbox("Tone Mapping", &_component.m_ToneMapping);
					ImGui::DragFloat("Scene Exposure", &_component.m_SceneExposure, 0.01f, 0.0f, 5.0f);

				});

			// ---------- Matériau du sprite en bas de l'inspecteur ----------
			if (_entity.HasComponent<SpriteRenderer>())
			{
				ImGui::PushID("SpriteMaterial");
				DrawMaterialInspector(_entity.GetComponent<SpriteRenderer>().m_Material, false);
				ImGui::PopID();
			}
		}
	}

}
