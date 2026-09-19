#pragma once
#include <Engine.h>
#include <Engine/Scene/Scene.h>

namespace Mayhem
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& _scene);

		void SetContext(const Ref<Scene>& _scene);

		void OnImGuiRender();
	private:
		void DrawEntityNode(Entity _entity);
		void DrawComponents(Entity _entity);
	private:
		Ref<Scene> m_Context;
		Entity m_selectionContext;
	};

}