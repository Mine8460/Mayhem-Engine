#pragma once

#include <Engine.h>
#include <chrono>
#include "Panels/SceneHierarchyPanel.h"

namespace Mayhem
{

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep _timestep) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& _e) override;

	private:

	private:
		Ref<Scene> m_ActiveScene;

		Ref<OrthographicCameraController> m_CameraController;

		Ref<Texture2D> m_Texture;
		Ref<Texture2D> m_AlphaTexture;
		Ref<Texture2D> m_SpriteSheet;
		std::vector<Ref<SubTexture2D>> m_Sprites;

		Ref<FrameBuffer> m_FrameBuffer;
		glm::vec2 m_ViewportSize;

		bool m_ViewportFocused = false;

		int spriteToUse = 1;

		glm::vec4 m_FlatColor = { 0.2f, 0.3f, 1.0f, 1.0f };

		struct ProfileResult
		{
			const char* Name;
			float Time;
		};

		std::vector<ProfileResult> m_ProfileResults;

		// Panels
		SceneHierarchyPanel m_Hierarchy;
	};
}