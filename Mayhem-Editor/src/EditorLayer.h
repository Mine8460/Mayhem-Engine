#pragma once

#include <Engine.h>
#include <chrono>
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include <Engine/Renderer/Camera/EditorCamera.h>
#include <imguizmo/src/ImGuizmo.h>

namespace Mayhem
{
	class EditorLayer : public Layer
	{
	public:
		enum SceneState
		{
			Editor,
			Runtime,
		};

		EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep _timestep) override;
		virtual void UI_Toolbar();
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& _e) override;
		virtual bool OnKeyPressed(KeyPressedEvent& _e);
		virtual bool OnMousePressed(MouseButtonPressedEvent& _e);

		void OnScenePlay();
		void OnSceneStop();

		void OpenScene(const std::filesystem::path& path);
		void OpenScene();
	private:

	private:
		Ref<Scene> m_ActiveScene;
		EditorCamera m_EditorCamera;

		Ref<Texture2D> m_Texture;
		Ref<Texture2D> m_AlphaTexture;
		Ref<Texture2D> m_SpriteSheet;
		std::vector<Ref<SubTexture2D>> m_Sprites;

		Ref<FrameBuffer> m_FrameBuffer;
		glm::vec2 m_ViewportSize;
		glm::vec2 m_ViewportBounds[2];

		Entity m_HoveredEntity;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		int spriteToUse = 1;

		glm::vec4 m_FlatColor = { 0.2f, 0.3f, 1.0f, 1.0f };

		struct ProfileResult
		{
			const char* Name;
			float Time;
		};

		int m_GizmoType = (int)ImGuizmo::OPERATION::TRANSLATE;

		std::vector<ProfileResult> m_ProfileResults;

		SceneState m_SceneState = SceneState::Editor;

		// Panels
		SceneHierarchyPanel m_Hierarchy;
		ContentBrowserPanel m_ContentBrowser;
	};
}