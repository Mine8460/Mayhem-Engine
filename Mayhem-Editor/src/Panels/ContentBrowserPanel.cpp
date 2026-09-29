#include "ContentBrowserPanel.h"
#include <imgui/imgui.h>
#include <Engine.h>
#include <cwchar>
#include <iostream>
#include <fstream>

#include "../EditorLayer.h"

namespace Mayhem
{
	// TO BE CHANGED
	constexpr const char* s_AssetsDirectory = "assets";

	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_CurrentDirectory = s_AssetsDirectory;

		m_DirectoryIcon = Texture2D::Create("assets/icons/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("assets/icons/FileIcon.png");
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != s_AssetsDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);
		for (auto& it : std::filesystem::directory_iterator(m_CurrentDirectory))
		{

			const auto& path = it.path();
			auto relativePath = std::filesystem::relative(path, s_AssetsDirectory);
			std::string filenameString = relativePath.filename().string();

			Ref<Texture2D> icon = it.is_directory() ? m_DirectoryIcon : m_FileIcon;

			std::string extension = relativePath.extension().string();

			if (extension == ".meta")
			{
				continue;
			}
			if (it.is_directory() == false)
			{

				if (extension == ".png")
				{
					std::string imagePath = path.string();
					icon = Texture2D::Create(imagePath);
				}
			}
			ImVec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

			float x = 1.0f;
			float y = 1.0f;
			float z = 1.0f;
			if (!m_FolderColors.contains(path.string()) && it.is_directory())
			{
				std::ifstream file(std::string(path.string() + ".meta").c_str(), std::ios::in);
				if (file)
				{
					file >> x;
					file >> y;
					file >> z;

				}
				m_FolderColors[path.string()] = glm::vec3(x, y, z);
				file.close();
			}

			if (it.is_directory())
			{
				glm::vec3 c = m_FolderColors[path.string()];
				color = { c.x, c.y, c.z, 1.0f };
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton(path.string().c_str(), (ImTextureID)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0,1 }, { 1,0 }, ImVec4(0, 0, 0, 0), color);

			std::string popupLabel = path.string() + "Folder Color";
			// Right - click blank space
			if (it.is_directory())
			{
				if (ImGui::BeginPopupContextItem(popupLabel.c_str(), ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
				{
					std::string label = "##" + path.string() + "Color";
					float colors[3] = { color.x, color.y, color.z };
					if (ImGui::ColorEdit3(label.c_str(), colors))
					{
						std::ofstream f(std::string(path.string() + ".meta").c_str(), std::ios::out);

						f << std::to_string(colors[0]) << std::endl;
						f << std::to_string(colors[1]) << std::endl;
						f << std::to_string(colors[2]) << std::endl;
						f.close();

						m_FolderColors[path.string()] = glm::vec3(colors[0], colors[1], colors[2]);
					}


					ImGui::EndPopup();
				}
			}

			if (ImGui::BeginDragDropSource())
			{
				const wchar_t* itemPath = path.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (std::wcslen(itemPath) + 1) * sizeof(wchar_t), ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (it.is_directory())
					m_CurrentDirectory /= path.filename();
				if (extension == ".mayhem")
				{
					if (m_Parent)
						m_Parent->OpenScene(path.c_str());
				}
			}
			ImGui::TextWrapped(filenameString.c_str());
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		ImGui::End();
	}
}
