#pragma once
#include <filesystem>
#include <string>
#include <Engine/Renderer/Texture.h>
#include <unordered_map>
namespace Mayhem
{
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		std::unordered_map<std::string, glm::vec3> m_FolderColors;
	};
}
