#include "SceneHierarchyPanel.h"

#include <glm/glm.hpp>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

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
					std::string name;
					if (_component.m_Texture)
					{
						name = _component.m_Texture->GetPath();
						if (ImGui::Button("X"))
							_component.m_Texture.reset();
						ImGui::SameLine();
					}
					else
						name = "None";

					ImGui::InputText("Texture", (char*)name.c_str(), name.size() * sizeof(char), ImGuiInputTextFlags_ReadOnly);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const wchar_t* path = (const wchar_t*)payload->Data;

							std::wstring ws = path;
							std::string pathString(ws.begin(), ws.end());

							_component.m_Texture = Texture2D::Create(pathString);

							ImGui::EndDragDropTarget();
						}

					};

					ImGui::DragFloat("Tiling factor", &_component.m_Tiling, 0.1f, 0.1f, 1000.f);
				});
		}
	}

}
