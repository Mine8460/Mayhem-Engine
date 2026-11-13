#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <Engine/Renderer/Texture.h>
#include "../ThumbnailRenderer.h"
#include <glm/glm.hpp>

namespace Mayhem
{
	class EditorLayer;

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void SetContext(EditorLayer* _parent) { m_Parent = _parent; }
		void OnImGuiRender();

		// ---------- Sélection (affichée dans l'inspecteur à la place d'une entité) ----------
		bool HasSelection() const { return !m_Selected.empty(); }
		const std::filesystem::path& GetSelection() const { return m_Selected; }
		void Select(const std::filesystem::path& _path);
		void ClearSelection() { m_Selected.clear(); }

		// Contenu de l'inspecteur pour l'élément sélectionné
		void DrawSelectionInspector();

	private:
		enum class AssetType { Folder, Scene, Prefab, Material, Texture, Mesh, Shader, Other };
		enum class NamePopupMode { None, CreateFolder, CreateMaterial, CreateScene, Rename };

		struct Entry
		{
			std::filesystem::path Path;
			bool IsDirectory = false;
			AssetType Type = AssetType::Other;
		};

		// ---------- Utilitaires ----------
		static AssetType GetAssetType(const std::filesystem::path& _path, bool _isDirectory);
		static const char* GetTypeName(AssetType _type);
		static bool IsValidFileName(const std::string& _name);
		static bool IsInside(const std::filesystem::path& _path, const std::filesystem::path& _parent);
		static std::string Key(const std::filesystem::path& _path);

		std::vector<Entry> ListDirectory() const;
		std::string GetUniqueName(const std::string& _base, const std::string& _extension) const;

		Ref<Texture2D> GetThumbnail(const Entry& _entry);
		glm::vec4 GetThumbnailTint(const Entry& _entry);
		glm::vec3 GetFolderColor(const std::filesystem::path& _folder);
		void SetFolderColor(const std::filesystem::path& _folder, const glm::vec3& _color);

		// ---------- Interface ----------
		void DrawBreadcrumbs();
		void DrawEntry(const Entry& _entry);
		void DrawBlankContextMenu();
		void DrawNamePopup();
		void DrawDeletePopup();
		void HandleShortcuts();
		// Cible de dépôt : fichier du Content Browser (= déplacer) ou entité de la hiérarchie (= créer un prefab)
		bool AcceptMoveDrop(const std::filesystem::path& _targetFolder);

		// ---------- Actions ----------
		void OpenNamePopup(NamePopupMode _mode, const std::string& _defaultName, const std::string& _extension,
			const std::filesystem::path& _target = {});
		void ApplyName(const std::string& _name);
		void RequestDelete(const std::filesystem::path& _path);
		void DeleteEntry(const std::filesystem::path& _path);
		void MoveEntry(const std::filesystem::path& _source, const std::filesystem::path& _targetFolder);
		void ForgetCached(const std::filesystem::path& _path);
		void CreatePrefab(uint64_t _entityUUID, const std::filesystem::path& _folder);
		void InstantiatePrefab(const std::filesystem::path& _path);

	private:
		std::filesystem::path m_CurrentDirectory;
		std::filesystem::path m_Selected;
		EditorLayer* m_Parent = nullptr;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
		std::unordered_map<std::string, glm::vec3> m_FolderColors;
		std::unordered_map<std::string, Ref<Texture2D>> m_ThumbnailCache;

		// Miniatures 3D (matériau sur sphère, aperçu de modèle) générées par ThumbnailRenderer
		struct GeneratedThumbnail
		{
			Ref<ThumbnailTexture> Texture;
			uint32_t Revision = 0;   // révision du matériau au moment du rendu
			bool Failed = false;     // chargement impossible : on n'insiste pas
			bool Deferred = false;   // très gros modèle pas encore en mémoire : attend qu'il soit chargé
		};
		std::unordered_map<std::string, GeneratedThumbnail> m_GeneratedThumbnails;
		int m_ThumbnailBudget = 0;   // miniatures 3D encore autorisées cette frame (évite les saccades)
		Ref<Texture2D> GetMaterialThumbnail(const Entry& _entry);
		Ref<Texture2D> GetMeshThumbnail(const Entry& _entry);

		float m_ThumbnailSize = 96.0f;

		// Actions différées (jamais pendant le parcours du dossier)
		bool m_HasPendingNavigate = false;
		std::filesystem::path m_PendingNavigate;
		bool m_HasPendingMove = false;
		std::filesystem::path m_PendingMoveSource;
		std::filesystem::path m_PendingMoveTarget;
		bool m_HasPendingPrefab = false;
		uint64_t m_PendingPrefabEntity = 0;
		std::filesystem::path m_PendingPrefabFolder;

		// Fenêtre de nom (créer / renommer)
		NamePopupMode m_NameMode = NamePopupMode::None;
		bool m_OpenNamePopup = false;
		char m_NameBuffer[128] = "";
		std::string m_NameExtension;
		std::filesystem::path m_NameTarget;

		// Suppression
		std::filesystem::path m_PendingDeletePath;
		size_t m_PendingDeleteCount = 0;
		bool m_OpenDeletePopup = false;
	};
}
