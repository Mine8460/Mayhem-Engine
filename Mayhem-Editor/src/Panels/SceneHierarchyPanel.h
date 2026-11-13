#pragma once
#include <Engine.h>
#include <filesystem>
#include <Engine/Scene/Scene.h>


namespace Mayhem
{
	class ContentBrowserPanel;

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& _scene);

		void SetContext(const Ref<Scene>& _scene);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_selectionContext; }
		void SetSelectedEntity(Entity _entity);
		void SetContentBrowser(ContentBrowserPanel* _contentBrowser) { m_ContentBrowser = _contentBrowser; }
	private:
		void DrawEntityNode(Entity _entity);
		void DrawComponents(Entity _entity);
	private:
		// Prefab lâché dans la hiérarchie : instancié après le parcours des entités
		bool m_HasPendingPrefab = false;
		std::filesystem::path m_PendingPrefabPath;
		uint64_t m_PendingPrefabParent = 0; // 0 = racine
		Ref<Scene> m_Context;
		ContentBrowserPanel* m_ContentBrowser = nullptr;
		Entity m_selectionContext;

		struct PendingActions
		{
			Entity Delete;
			bool Reparent = false;
			uint64_t ReparentChild = 0;
			uint64_t ReparentParent = 0;   // 0 = détacher
			bool CreateChild = false;
			uint64_t CreateChildOf = 0;
		};
		PendingActions m_Pending;
	};

}