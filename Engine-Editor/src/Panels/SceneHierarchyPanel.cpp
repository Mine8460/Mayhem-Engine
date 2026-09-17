#include "SceneHierarchyPanel.h"

#include <glm/glm.hpp>
#include <imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
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

			ImGui::Separator();

			if (ImGui::Button("Add Component"))
				ImGui::OpenPopup("ComponentsList");

			if (ImGui::BeginPopup("ComponentsList"))
			{
				if (ImGui::MenuItem("Camera"))
				{
					m_selectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Sprite"))
				{
					m_selectionContext.AddComponent<SpriteRenderer>();
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity _entity)
	{
		TagComponent tag = _entity.GetComponent<TagComponent>();

		ImGuiTreeNodeFlags selected = (m_selectionContext == _entity) ? ImGuiTreeNodeFlags_Selected : 0;
		ImGuiTreeNodeFlags flags = selected | ImGuiTreeNodeFlags_OpenOnArrow;
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
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.x = _reset;
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
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.y = _reset;
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
		if (ImGui::Button(button.c_str(), ButtonSize))
			values.z = _reset;
		ImGui::SameLine();
		ImGui::DragFloat(label.c_str(), &values.z, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();

		ImGui::Columns(1);
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
				ImGui::Text("Tag");
				std::string label = "##" + std::to_string((uint32_t)_entity);
				if (ImGui::InputText(label.c_str(), buffer, sizeof(char) * 256))
				{
					tag = std::string(buffer);
				}
			}

			if (_entity.HasComponent<TransformComponent>())
			{
				if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
				{
					auto& tc = _entity.GetComponent<TransformComponent>();

					std::string label = "##" + std::to_string((uint32_t)_entity);
					DrawVec3Control("Position", tc.m_Translation);
					DrawVec3Control("Rotation", tc.m_Rotation);
					DrawVec3Control("Scale", tc.m_Scale, 1.0f);

					ImGui::TreePop();
				}
			}

			const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;

			if (_entity.HasComponent<CameraComponent>())
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

				bool open = ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), treeFlags, "Camera");
				ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
				if (ImGui::Button("+", ImVec2{ 20,20 }))
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
					auto* camera = &_entity.GetComponent<CameraComponent>();

					bool primary = camera->m_Primary;
					if (ImGui::Checkbox("Primary", &primary))
						camera->m_Primary = primary;

					ImGui::Separator();

					const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
					const char* currentProjection = projectionTypeStrings[(int)camera->m_Camera.GetProjectionType()];
					if (ImGui::BeginCombo("Projection", currentProjection))
					{
						for (int i = 0; i < 2; i++)
						{
							bool isSelected = currentProjection == projectionTypeStrings[i];

							if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
							{
								currentProjection = projectionTypeStrings[i];
								camera->m_Camera.SetProjectionType((SceneCamera::ProjectionType)i);
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					if (camera->m_Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
					{
						float fov = glm::degrees(camera->m_Camera.GetFOV());
						float nearClip = camera->m_Camera.GetPerspectiveNearClip();
						float farClip = camera->m_Camera.GetPerspectiveFarClip();

						if (ImGui::DragFloat("Field of view", &fov, 1.f))
							camera->m_Camera.SetFOV(glm::radians(fov));
						if (ImGui::DragFloat("Near clip", &nearClip, 1.f))
							camera->m_Camera.SetPerspectiveNearClip(nearClip);
						if (ImGui::DragFloat("Far clip", &farClip, 10.f))
							camera->m_Camera.SetPerspectiveFarClip(farClip);
					}
					if (camera->m_Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
					{
						float zoom = camera->m_Camera.GetOrthographicSize();
						float nearClip = camera->m_Camera.GetOrthographicNearClip();
						float farClip = camera->m_Camera.GetOrthographicFarClip();

						if (ImGui::DragFloat("Zoom", &zoom, 0.25f, 1.0f, 50.f))
							camera->m_Camera.SetOrthographicSize(zoom);
						if (ImGui::DragFloat("Near clip", &nearClip, 1.f))
							camera->m_Camera.SetOrthographicNearClip(nearClip);
						if (ImGui::DragFloat("Far clip", &farClip, 10.f))
							camera->m_Camera.SetOrthographicFarClip(farClip);

						ImGui::Separator();

						if (ImGui::Checkbox("Fixed aspect ratio", &camera->m_FixedAspectRatio))
							camera->m_Camera.SetOrthographicSize(camera->m_Camera.GetOrthographicSize());
					}
					ImGui::TreePop();

					if (remove)
					{
						_entity.RemoveComponent<CameraComponent>();
					}

				}
				ImGui::PopStyleVar();
			}

			if (_entity.HasComponent<SpriteRenderer>())
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

				bool open = ImGui::TreeNodeEx((void*)typeid(SpriteRenderer).hash_code(), treeFlags, "Sprite");
				ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
				if (ImGui::Button("+", ImVec2{ 20,20 }))
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
					auto* sprite = &_entity.GetComponent<SpriteRenderer>();

					std::string name = "##Color" + std::to_string((uint32_t)_entity);
					ImGui::ColorEdit4(name.c_str(), glm::value_ptr(sprite->m_Color));

					ImGui::TreePop();
				}

				if (remove)
				{
					_entity.RemoveComponent<SpriteRenderer>();
				}
				ImGui::PopStyleVar();
			}
		}
	}

}
