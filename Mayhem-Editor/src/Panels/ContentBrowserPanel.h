#pragma once
#include <filesystem>
#include <string>
#include <Engine/Renderer/Texture.h>
#include <unordered_map>
namespace Mayhem
{
	class EditorLayer;
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void SetContext(EditorLayer* _parent) { m_Parent = _parent; }
		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;

		EditorLayer* m_Parent;
		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		std::unordered_map<std::string, glm::vec3> m_FolderColors;
	};
}
