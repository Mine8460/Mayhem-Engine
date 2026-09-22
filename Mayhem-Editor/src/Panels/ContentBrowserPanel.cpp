#include "ContentBrowserPanel.h"
#include <imgui/imgui.h>
#include <Engine.h>
#include <cwchar>
#include <iostream>

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
			if (!m_FolderColors.contains(path.string()))
			{
				m_FolderColors[path.string()] = glm::vec3(1.0f, 1.0f, 1.0f);
			}
			auto relativePath = std::filesystem::relative(path, s_AssetsDirectory);
			std::string filenameString = relativePath.filename().string();

			Ref<Texture2D> icon = it.is_directory() ? m_DirectoryIcon : m_FileIcon;

			if (it.is_directory() == false)
			{
				std::string extension = relativePath.extension().string();

				if (extension == ".png")
				{
					std::string imagePath = path.string();
					icon = Texture2D::Create(imagePath);
				}
			}

			glm::vec3 folderColor = m_FolderColors[path.string()];
			ImVec4 color = { folderColor.x, folderColor.y, folderColor.z, 1.0f };
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton(path.string().c_str(), (ImTextureID)icon->GetRendererID(), { thumbnailSize, thumbnailSize }, { 0,1 }, { 1,0 }, ImVec4(0, 0, 0, 0), color);

			std::string popupLabel = path.string() + "Folder Color";
			// Right - click blank space
			if (it.is_directory())
				if (ImGui::BeginPopupContextItem(popupLabel.c_str(), ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
				{
					std::string label = "##" + path.string() + "Color";
					float colors[3] = { folderColor.x, folderColor.y, folderColor.z };
					ImGui::ColorEdit3(label.c_str(), colors);
					m_FolderColors[path.string()] = { colors[0], colors[1], colors[2] };

					ImGui::EndPopup();
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
			}
			ImGui::TextWrapped(filenameString.c_str());
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		ImGui::End();
	}
}
