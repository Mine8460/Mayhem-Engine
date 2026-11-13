#pragma once

#include <Engine.h>
#include <chrono>
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include <Engine/Renderer/Camera/EditorCamera.h>
#include <Engine/Renderer/SceneRenderer.h>
#include <imguizmo/src/ImGuizmo.h>
#include <filesystem>
#include <cctype>
#include "UndoHistory.h"
#include "ViewportIcons.h"
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

		enum class PendingSceneAction { None, New, Open, OpenPath, Exit };

		void RequestSceneAction(PendingSceneAction _action, const std::filesystem::path& _path = {});
		void ExecuteSceneAction(PendingSceneAction _action, const std::filesystem::path& _path);
		void MarkSceneSaved();
		void UpdateDirtyState();
		std::string GetSceneDisplayName() const;
		void UI_UnsavedChangesPopup();

		EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep _timestep) override;
		virtual void UI_Toolbar();
		void UI_GizmoOverlay();
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& _e) override;
		virtual bool OnKeyPressed(KeyPressedEvent& _e);
		virtual bool OnMousePressed(MouseButtonPressedEvent& _e);

		inline std::string GetExtension(const std::string& _path)
		{
			std::string ext = std::filesystem::path(_path).extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return (char)std::tolower(c); });
			return ext;
		}
		void ClearEntitySelection() { m_Hierarchy.SetSelectedEntity(Entity{}); }

		void OnScenePlay();
		void OnSceneStop();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& _path);
		void SaveScene();
		void SaveSceneAs();

		void OnDuplicateEntity();

		void OnUndo();
		void OnRedo();
		void SetEditorScene(const Ref<Scene>& _scene);

		Ref<Scene> GetActiveScene() const { return m_ActiveScene; }
		// Ajoute le prefab dans la scène et le sélectionne
		Entity InstantiatePrefab(const std::filesystem::path& _path, Entity _parent = {});
	private:
		std::string m_SavedSceneText;          // la scène telle qu'elle est sur le disque
		bool m_SceneDirty = false;
		uint64_t m_CheckedVersion = (uint64_t)-1;

		PendingSceneAction m_PendingAction = PendingSceneAction::None;
		std::filesystem::path m_PendingPath;
		bool m_OpenUnsavedPopup = false;
	private:
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;              // la scène éditée, mise de côté pendant le mode Play
		std::filesystem::path m_EditorScenePath; // fichier de la scène ouverte (vide = jamais sauvegardée)
		EditorCamera m_EditorCamera;
		UndoHistory m_Undo;

		Ref<Texture2D> m_Texture;
		Ref<Texture2D> m_AlphaTexture;
		Ref<Texture2D> m_SpriteSheet;
		std::vector<Ref<SubTexture2D>> m_Sprites;

		Ref<FrameBuffer> m_FrameBuffer;
		glm::vec2 m_ViewportSize;
		glm::vec2 m_ViewportBounds[2];

		Entity m_HoveredEntity;
		ViewportIcons m_ViewportIcons;
		Entity m_HoveredIcon;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		bool m_ShowGrid = true;
		bool m_ShowWireframe = false; // arêtes des modèles par-dessus la scène
		bool m_BlockGizmoUntilRelease = false;

		int spriteToUse = 1;

		glm::vec4 m_FlatColor = { 0.2f, 0.3f, 1.0f, 1.0f };

		struct ProfileResult
		{
			const char* Name;
			float Time;
		};

		int m_GizmoType = (int)ImGuizmo::OPERATION::TRANSLATE;
		int m_GizmoMode = (int)ImGuizmo::MODE::LOCAL;

		std::vector<ProfileResult> m_ProfileResults;

		SceneState m_SceneState = SceneState::Editor;

		// Panels
		SceneHierarchyPanel m_Hierarchy;
		ContentBrowserPanel m_ContentBrowser;
	};
}