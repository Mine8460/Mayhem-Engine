#include "ContentBrowserPanel.h"
#include <imgui/imgui.h>
#include <Engine.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Core/AssetsManager.h>
#include <Engine/Scene/Prefab.h>
#include <glm/gtc/type_ptr.hpp>

#include <cwchar>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>

#include "../EditorLayer.h"
#include "../EditorWidgets.h"

#ifdef _WIN32
	#ifndef NOMINMAX
		#define NOMINMAX            // windows.h définit min/max en macros sinon
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <shellapi.h>
#endif

namespace Mayhem
{
	// TO BE CHANGED
	constexpr const char* s_AssetsDirectory = "assets";

	static const ImU32 s_SelectedColor = IM_COL32(140, 70, 210, 110); // violet Mayhem
	static const ImU32 s_HoveredColor = IM_COL32(255, 255, 255, 22);
	static const ImU32 s_DropColor = IM_COL32(179, 89, 255, 255);

	// =====================================================================
	//  Utilitaires
	// =====================================================================

	// Envoie un fichier ou un dossier (avec tout son contenu) dans la corbeille
	static bool MoveToRecycleBin(const std::filesystem::path& _path)
	{
#ifdef _WIN32
		std::wstring from = std::filesystem::absolute(_path).wstring();
		from.push_back(L'\0'); // SHFileOperation attend une liste terminée par un double zéro

		SHFILEOPSTRUCTW operation = {};
		operation.wFunc = FO_DELETE;
		operation.pFrom = from.c_str();
		operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

		return SHFileOperationW(&operation) == 0 && !operation.fAnyOperationsAborted;
#else
		std::error_code error;
		std::filesystem::remove_all(_path, error); // pas de corbeille hors Windows
		return !error;
#endif
	}

	static std::string ToLower(std::string _s)
	{
		for (auto& c : _s) c = (char)std::tolower((unsigned char)c);
		return _s;
	}

	static std::filesystem::path MetaPath(const std::filesystem::path& _folder)
	{
		return std::filesystem::path(_folder.string() + ".meta");
	}

	static void WriteFolderMeta(const std::filesystem::path& _folder, const glm::vec3& _color)
	{
		std::ofstream meta(MetaPath(_folder), std::ios::out);
		meta << std::to_string(_color.r) << std::endl;
		meta << std::to_string(_color.g) << std::endl;
		meta << std::to_string(_color.b) << std::endl;
	}

	static std::string FormatFileSize(uintmax_t _bytes)
	{
		char buffer[32];
		if (_bytes < 1024)
			snprintf(buffer, sizeof(buffer), "%llu o", (unsigned long long)_bytes);
		else if (_bytes < 1024 * 1024)
			snprintf(buffer, sizeof(buffer), "%.1f Ko", _bytes / 1024.0);
		else
			snprintf(buffer, sizeof(buffer), "%.1f Mo", _bytes / (1024.0 * 1024.0));
		return buffer;
	}

	std::string ContentBrowserPanel::Key(const std::filesystem::path& _path)
	{
		return _path.lexically_normal().generic_string();
	}

	ContentBrowserPanel::AssetType ContentBrowserPanel::GetAssetType(const std::filesystem::path& _path, bool _isDirectory)
	{
		if (_isDirectory)
			return AssetType::Folder;

		std::string ext = ToLower(_path.extension().string());
		if (ext == ".mayhem") return AssetType::Scene;
		if (ext == Prefab::Extension) return AssetType::Prefab;
		if (ext == ".mat") return AssetType::Material;
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") return AssetType::Texture;
		if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") return AssetType::Mesh;
		if (ext == ".glsl") return AssetType::Shader;
		return AssetType::Other;
	}

	const char* ContentBrowserPanel::GetTypeName(AssetType _type)
	{
		switch (_type)
		{
		case AssetType::Folder:   return "Dossier";
		case AssetType::Scene:    return "Scene";
		case AssetType::Prefab:   return "Prefab";
		case AssetType::Material: return "Materiau";
		case AssetType::Texture:  return "Texture";
		case AssetType::Mesh:     return "Modele 3D";
		case AssetType::Shader:   return "Shader";
		default:                  return "Fichier";
		}
	}

	bool ContentBrowserPanel::IsValidFileName(const std::string& _name)
	{
		if (_name.empty() || _name.find_first_not_of(' ') == std::string::npos)
			return false;
		if (_name.back() == '.' || _name.back() == ' ') // refusés par Windows en fin de nom
			return false;
		return _name.find_first_of("\\/:*?\"<>|") == std::string::npos;
	}

	// _path est _parent lui-même ou se trouve dedans
	bool ContentBrowserPanel::IsInside(const std::filesystem::path& _path, const std::filesystem::path& _parent)
	{
		std::filesystem::path relative = _path.lexically_normal().lexically_relative(_parent.lexically_normal());
		return !relative.empty() && *relative.begin() != "..";
	}

	// "Nouveau dossier", puis "Nouveau dossier 1"... si le nom est pris
	std::string ContentBrowserPanel::GetUniqueName(const std::string& _base, const std::string& _extension) const
	{
		std::string name = _base;
		for (int i = 1; std::filesystem::exists(m_CurrentDirectory / (name + _extension)); i++)
			name = _base + " " + std::to_string(i);
		return name;
	}

	std::vector<ContentBrowserPanel::Entry> ContentBrowserPanel::ListDirectory() const
	{
		std::vector<Entry> entries;
		std::error_code error;

		for (auto& item : std::filesystem::directory_iterator(m_CurrentDirectory, error))
		{
			const std::filesystem::path& path = item.path();
			const bool isDirectory = item.is_directory();

			if (path.extension() == ".meta")
				continue;

			// Seuls les dossiers gérés par l'éditeur (avec un .meta) sont affichés
			if (isDirectory && !std::filesystem::exists(MetaPath(path)))
				continue;

			// Chaque fichier a son identifiant (crée le .meta s'il manque, ex : fichier ajouté depuis l'explorateur)
			if (!isDirectory)
				AssetsManager::GetID(Key(path));

			entries.push_back({ path, isDirectory, GetAssetType(path, isDirectory) });
		}

		// Dossiers d'abord, puis ordre alphabétique (insensible à la casse)
		std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b)
			{
				if (a.IsDirectory != b.IsDirectory)
					return a.IsDirectory;
				return ToLower(a.Path.filename().string()) < ToLower(b.Path.filename().string());
			});

		return entries;
	}

	// =====================================================================
	//  Miniatures et couleurs
	// =====================================================================

	glm::vec3 ContentBrowserPanel::GetFolderColor(const std::filesystem::path& _folder)
	{
		const std::string key = Key(_folder);
		auto it = m_FolderColors.find(key);
		if (it != m_FolderColors.end())
			return it->second;

		glm::vec3 color(1.0f);
		std::ifstream file(MetaPath(_folder), std::ios::in);
		if (file)
			file >> color.r >> color.g >> color.b;

		m_FolderColors[key] = color;
		return color;
	}

	void ContentBrowserPanel::SetFolderColor(const std::filesystem::path& _folder, const glm::vec3& _color)
	{
		WriteFolderMeta(_folder, _color);
		m_FolderColors[Key(_folder)] = _color;
	}

	Ref<Texture2D> ContentBrowserPanel::GetThumbnail(const Entry& _entry)
	{
		switch (_entry.Type)
		{
		case AssetType::Folder:
			return m_DirectoryIcon;

		case AssetType::Texture:
		{
			// Chargée une seule fois, puis réutilisée
			const std::string key = Key(_entry.Path);
			auto cached = m_ThumbnailCache.find(key);
			if (cached == m_ThumbnailCache.end())
				cached = m_ThumbnailCache.emplace(key, Texture2D::Create(_entry.Path.string())).first;
			return cached->second ? cached->second : m_FileIcon;
		}

		case AssetType::Material:
			return GetMaterialThumbnail(_entry);

		case AssetType::Mesh:
			return GetMeshThumbnail(_entry);

		default:
			return m_FileIcon;
		}
	}

	// Le matériau appliqué sur une sphère, redessinée à chaque sauvegarde du matériau
	Ref<Texture2D> ContentBrowserPanel::GetMaterialThumbnail(const Entry& _entry)
	{
		const std::string key = Key(_entry.Path);
		GeneratedThumbnail& thumbnail = m_GeneratedThumbnails[key];
		if (thumbnail.Failed)
			return m_FileIcon;

		Ref<Material> material = Material::Load(key);
		if (!material)
		{
			thumbnail.Failed = true;
			return m_FileIcon;
		}

		const bool outdated = !thumbnail.Texture || thumbnail.Revision != material->GetRevision();
		if (outdated && m_ThumbnailBudget > 0)
		{
			m_ThumbnailBudget--;
			thumbnail.Texture = ThumbnailRenderer::RenderMaterial(material, thumbnail.Texture);
			thumbnail.Revision = material->GetRevision();
		}

		// Pas encore dessinée (budget de la frame épuisé) : icône en attendant
		return thumbnail.Texture ? Ref<Texture2D>(thumbnail.Texture) : m_FileIcon;
	}

	// Aperçu du modèle. Une seule fois : un modèle ne change pas pendant que l'éditeur tourne.
	Ref<Texture2D> ContentBrowserPanel::GetMeshThumbnail(const Entry& _entry)
	{
		const std::string key = Key(_entry.Path);
		GeneratedThumbnail& thumbnail = m_GeneratedThumbnails[key];
		if (thumbnail.Failed)
			return m_FileIcon;
		if (thumbnail.Texture)
			return thumbnail.Texture;

		// Importer un énorme fichier 3D pour la première fois gèlerait l'éditeur : on attend qu'il soit
		// déjà en mémoire (ex : utilisé par la scène) et on garde l'icône en attendant.
		constexpr uintmax_t largeFileBytes = 24ull * 1024 * 1024;
		std::error_code error;
		const uintmax_t fileSize = std::filesystem::file_size(_entry.Path, error);
		if (!error && fileSize > largeFileBytes && !Mesh::GetLoaded(key))
		{
			thumbnail.Deferred = true;
			return m_FileIcon;
		}

		if (m_ThumbnailBudget <= 0)
			return m_FileIcon;
		m_ThumbnailBudget--;

		if (Ref<Mesh> mesh = Mesh::Create(key))
			thumbnail.Texture = ThumbnailRenderer::RenderMesh(mesh);
		thumbnail.Failed = !thumbnail.Texture;
		return thumbnail.Texture ? Ref<Texture2D>(thumbnail.Texture) : m_FileIcon;
	}

	glm::vec4 ContentBrowserPanel::GetThumbnailTint(const Entry& _entry)
	{
		if (_entry.Type == AssetType::Folder)
			return glm::vec4(GetFolderColor(_entry.Path), 1.0f);

		// Même couleur que les instances dans la hiérarchie
		if (_entry.Type == AssetType::Prefab)
			return EditorUI::GetPrefabColor();

		return glm::vec4(1.0f);
	}

	// =====================================================================
	//  Sélection
	// =====================================================================

	void ContentBrowserPanel::Select(const std::filesystem::path& _path)
	{
		// Sélectionner un asset désélectionne l'entité : l'inspecteur montre l'asset
		if (m_Parent)
			m_Parent->ClearEntitySelection();

		m_Selected = _path.lexically_normal();
	}

	// =====================================================================
	//  Actions
	// =====================================================================

	void ContentBrowserPanel::ForgetCached(const std::filesystem::path& _path)
	{
		for (auto it = m_FolderColors.begin(); it != m_FolderColors.end(); )
			it = IsInside(it->first, _path) ? m_FolderColors.erase(it) : std::next(it);

		for (auto it = m_ThumbnailCache.begin(); it != m_ThumbnailCache.end(); )
			it = IsInside(it->first, _path) ? m_ThumbnailCache.erase(it) : std::next(it);

		for (auto it = m_GeneratedThumbnails.begin(); it != m_GeneratedThumbnails.end(); )
			it = IsInside(it->first, _path) ? m_GeneratedThumbnails.erase(it) : std::next(it);
	}

	void ContentBrowserPanel::CreatePrefab(uint64_t _entityUUID, const std::filesystem::path& _folder)
	{
		if (!m_Parent || !m_Parent->GetActiveScene())
			return;

		Entity entity = m_Parent->GetActiveScene()->GetEntityByUUID(_entityUUID);
		if (!entity)
			return;

		Prefab::CreateFromEntity(entity, _folder);
	}

	void ContentBrowserPanel::InstantiatePrefab(const std::filesystem::path& _path)
	{
		if (m_Parent)
			m_Parent->InstantiatePrefab(_path);
	}

	void ContentBrowserPanel::OpenNamePopup(NamePopupMode _mode, const std::string& _defaultName, const std::string& _extension,
		const std::filesystem::path& _target)
	{
		m_NameMode = _mode;
		strncpy_s(m_NameBuffer, sizeof(m_NameBuffer), _defaultName.c_str(), _TRUNCATE);
		m_NameExtension = _extension;
		m_NameTarget = _target;
		m_OpenNamePopup = true;
	}

	void ContentBrowserPanel::ApplyName(const std::string& _name)
	{
		std::error_code error;

		if (m_NameMode == NamePopupMode::Rename)
		{
			const std::filesystem::path destination = m_NameTarget.parent_path() / (_name + m_NameExtension);

			std::filesystem::rename(m_NameTarget, destination, error);
			if (error)
				return;

			// Le .meta suit (couleur d'un dossier, identifiant d'un fichier)
			if (std::filesystem::exists(MetaPath(m_NameTarget)))
				std::filesystem::rename(MetaPath(m_NameTarget), MetaPath(destination), error);

			AssetsManager::OnMoved(m_NameTarget, destination);
			ForgetCached(m_NameTarget);

			if (m_Selected == m_NameTarget.lexically_normal())
				m_Selected = destination.lexically_normal();
			else if (IsInside(m_Selected, m_NameTarget))
				m_Selected.clear();
			return;
		}

		const std::filesystem::path destination = m_CurrentDirectory / (_name + m_NameExtension);

		switch (m_NameMode)
		{
		case NamePopupMode::CreateFolder:
			if (!std::filesystem::create_directory(destination, error))
				return;
			WriteFolderMeta(destination, glm::vec3(1.0f)); // le .meta = dossier géré par l'éditeur
			break;

		case NamePopupMode::CreateMaterial:
			if (!Material::CreateAsset(Key(destination)))
				return;
			break;

		case NamePopupMode::CreateScene:
		{
			Ref<Scene> scene = MakeRef<Scene>();
			SceneSerializer serializer(scene);
			serializer.Serialize(destination.string());
			break;
		}

		default:
			return;
		}

		Select(destination);
	}

	void ContentBrowserPanel::RequestDelete(const std::filesystem::path& _path)
	{
		m_PendingDeletePath = _path;
		m_PendingDeleteCount = 0;

		if (std::filesystem::is_directory(_path))
		{
			std::error_code error;
			for (auto& entry : std::filesystem::recursive_directory_iterator(_path, error))
				if (entry.is_regular_file() && entry.path().extension() != ".meta")
					m_PendingDeleteCount++;
		}

		m_OpenDeletePopup = true;
	}

	void ContentBrowserPanel::DeleteEntry(const std::filesystem::path& _path)
	{
		if (!MoveToRecycleBin(_path))
			return;

		// Le .meta part avec lui (couleur d'un dossier, identifiant d'un fichier)
		if (std::filesystem::exists(MetaPath(_path)))
			MoveToRecycleBin(MetaPath(_path));

		AssetsManager::OnDeleted(_path);
		ForgetCached(_path);

		if (IsInside(m_Selected, _path))
			m_Selected.clear();
	}

	void ContentBrowserPanel::MoveEntry(const std::filesystem::path& _source, const std::filesystem::path& _targetFolder)
	{
		if (!std::filesystem::exists(_source))
			return;

		// Déjà dans ce dossier, ou dossier déplacé dans lui-même / un de ses sous-dossiers
		if (_source.parent_path().lexically_normal() == _targetFolder.lexically_normal() || IsInside(_targetFolder, _source))
			return;

		const std::filesystem::path destination = _targetFolder / _source.filename();
		if (std::filesystem::exists(destination))
			return; // un élément porte déjà ce nom là-bas

		std::error_code error;
		std::filesystem::rename(_source, destination, error);
		if (error)
			return;

		// Le .meta suit (couleur d'un dossier, identifiant d'un fichier)
		if (std::filesystem::exists(MetaPath(_source)))
			std::filesystem::rename(MetaPath(_source), MetaPath(destination), error);

		AssetsManager::OnMoved(_source, destination);
		ForgetCached(_source);

		if (m_Selected == _source.lexically_normal())
			m_Selected = destination.lexically_normal();
		else if (IsInside(m_Selected, _source))
			m_Selected.clear();
	}

	// =====================================================================
	//  Interface
	// =====================================================================

	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_CurrentDirectory = s_AssetsDirectory;

		m_DirectoryIcon = Texture2D::Create("assets/icons/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("assets/icons/FileIcon.png");
	}

	// Cible de dépôt pour déplacer un élément du Content Browser dans _targetFolder
	bool ContentBrowserPanel::AcceptMoveDrop(const std::filesystem::path& _targetFolder)
	{
		bool accepted = false;
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				m_PendingMoveSource = std::filesystem::path((const wchar_t*)payload->Data);
				m_PendingMoveTarget = _targetFolder;
				m_HasPendingMove = true;
				accepted = true;
			}

			// Entité glissée depuis la hiérarchie : nouveau prefab dans ce dossier
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
			{
				m_PendingPrefabEntity = *(const uint64_t*)payload->Data;
				m_PendingPrefabFolder = _targetFolder;
				m_HasPendingPrefab = true;
				accepted = true;
			}
			ImGui::EndDragDropTarget();
		}
		return accepted;
	}

	void ContentBrowserPanel::DrawBreadcrumbs()
	{
		// assets > materials > metal
		std::filesystem::path relative = m_CurrentDirectory.lexically_normal().lexically_relative(s_AssetsDirectory);
		std::filesystem::path accumulated = s_AssetsDirectory;

		std::vector<std::filesystem::path> crumbs = { accumulated };
		for (const auto& part : relative)
		{
			if (part == ".")
				continue;
			accumulated /= part;
			crumbs.push_back(accumulated);
		}

		for (size_t i = 0; i < crumbs.size(); i++)
		{
			if (i > 0)
			{
				ImGui::SameLine();
				ImGui::TextDisabled(">");
				ImGui::SameLine();
			}

			ImGui::PushID((int)i);
			const bool isCurrent = (i == crumbs.size() - 1);
			std::string label = crumbs[i].filename().string();

			if (isCurrent)
			{
				ImGui::Text("%s", label.c_str());
			}
			else
			{
				if (ImGui::SmallButton(label.c_str()))
				{
					m_PendingNavigate = crumbs[i];
					m_HasPendingNavigate = true;
				}
				AcceptMoveDrop(crumbs[i]); // glisser sur un parent = déplacer dedans
			}
			ImGui::PopID();
		}

		ImGui::Separator();
	}

	void ContentBrowserPanel::DrawEntry(const Entry& _entry)
	{
		const std::string key = Key(_entry.Path);
		const std::string fileName = _entry.Path.filename().string();
		const std::string displayName = _entry.IsDirectory ? fileName : _entry.Path.stem().string();
		const ImVec2 size(m_ThumbnailSize, m_ThumbnailSize);

		ImGui::PushID(key.c_str());

		const ImVec2 position = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##Item", size);
		const bool hovered = ImGui::IsItemHovered();

		// Sélection au relâchement, et seulement si ce n'était pas un glisser-déposer
		const float dragThreshold = ImGui::GetIO().MouseDragThreshold;
		const bool wasDragged = ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] >= dragThreshold * dragThreshold;
		if (ImGui::IsItemDeactivated() && !wasDragged)
			Select(_entry.Path);

		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (_entry.IsDirectory)
			{
				m_PendingNavigate = _entry.Path;
				m_HasPendingNavigate = true;
			}
			else if (_entry.Type == AssetType::Scene && m_Parent)
			{
				m_Parent->RequestSceneAction(EditorLayer::PendingSceneAction::OpenPath, _entry.Path);
			}
			else if (_entry.Type == AssetType::Prefab)
			{
				InstantiatePrefab(_entry.Path);
			}
		}

		// Clic droit sur l'élément
		if (ImGui::BeginPopupContextItem("##ItemContext"))
		{
			if (ImGui::IsWindowAppearing())
				Select(_entry.Path);

			if (_entry.Type == AssetType::Scene && ImGui::MenuItem("Ouvrir") && m_Parent)
				m_Parent->RequestSceneAction(EditorLayer::PendingSceneAction::OpenPath, _entry.Path);

			if (_entry.Type == AssetType::Prefab && ImGui::MenuItem("Ajouter a la scene"))
				InstantiatePrefab(_entry.Path);

			if (ImGui::MenuItem("Renommer", "F2"))
				OpenNamePopup(NamePopupMode::Rename, displayName, _entry.IsDirectory ? "" : _entry.Path.extension().string(), _entry.Path);

			ImGui::Separator();
			if (ImGui::MenuItem("Supprimer", "Suppr"))
				RequestDelete(_entry.Path);

			ImGui::EndPopup();
		}

		// Glisser l'élément (vers le viewport, l'inspecteur, ou un dossier)
		if (ImGui::BeginDragDropSource())
		{
			const wchar_t* itemPath = _entry.Path.c_str();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (std::wcslen(itemPath) + 1) * sizeof(wchar_t), ImGuiCond_Always);
			ImGui::Text("%s", fileName.c_str());
			ImGui::EndDragDropSource();
		}

		// Lâcher sur un dossier = déplacer dedans
		bool dropHovered = false;
		if (_entry.IsDirectory)
		{
			const ImGuiPayload* payload = ImGui::GetDragDropPayload();
			dropHovered = hovered && payload && (payload->IsDataType("CONTENT_BROWSER_ITEM") || payload->IsDataType("SCENE_ENTITY"));
			AcceptMoveDrop(_entry.Path);
		}

		// ---------- Dessin ----------
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 end(position.x + size.x, position.y + size.y);

		if (m_Selected == _entry.Path.lexically_normal())
			drawList->AddRectFilled(position, end, s_SelectedColor, 6.0f);
		else if (hovered)
			drawList->AddRectFilled(position, end, s_HoveredColor, 6.0f);

		if (dropHovered)
			drawList->AddRect(position, end, s_DropColor, 6.0f, 0, 2.0f);

		Ref<Texture2D> thumbnail = GetThumbnail(_entry);
		const glm::vec4 tint = GetThumbnailTint(_entry);
		const float padding = 8.0f;
		drawList->AddImage((ImTextureID)(uintptr_t)thumbnail->GetRendererID(),
			ImVec2(position.x + padding, position.y + padding), ImVec2(end.x - padding, end.y - padding),
			ImVec2(0, 1), ImVec2(1, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(tint.r, tint.g, tint.b, tint.a)));

		// Nom (sans extension pour les fichiers, nom complet au survol)
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + m_ThumbnailSize);
		ImGui::TextWrapped("%s", displayName.c_str());
		ImGui::PopTextWrapPos();
		if (hovered && !_entry.IsDirectory)
			ImGui::SetTooltip("%s", fileName.c_str());

		ImGui::PopID();
	}

	void ContentBrowserPanel::DrawBlankContextMenu()
	{
		if (ImGui::BeginPopupContextWindow("##BlankContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::BeginMenu("Creer"))
			{
				if (ImGui::MenuItem("Dossier"))
					OpenNamePopup(NamePopupMode::CreateFolder, GetUniqueName("Nouveau dossier", ""), "");

				ImGui::Separator();

				if (ImGui::MenuItem("Materiau"))
					OpenNamePopup(NamePopupMode::CreateMaterial, GetUniqueName("NouveauMateriau", ".mat"), ".mat");

				if (ImGui::MenuItem("Scene"))
					OpenNamePopup(NamePopupMode::CreateScene, GetUniqueName("NouvelleScene", ".mayhem"), ".mayhem");

				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::DrawNamePopup()
	{
		const char* popupName = "Nom##ContentBrowserNamePopup";

		if (m_OpenNamePopup)
		{
			ImGui::OpenPopup(popupName);
			m_OpenNamePopup = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		const bool renaming = m_NameMode == NamePopupMode::Rename;
		ImGui::Text(renaming ? "Nouveau nom :" : "Nom :");

		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere(); // on peut taper directement

		ImGui::SetNextItemWidth(260.0f);
		bool confirm = ImGui::InputText("##Name", m_NameBuffer, sizeof(m_NameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

		if (!m_NameExtension.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("%s", m_NameExtension.c_str()); // l'extension n'est pas modifiable
		}

		const std::string name = m_NameBuffer;
		const std::filesystem::path folder = renaming ? m_NameTarget.parent_path() : m_CurrentDirectory;
		const std::filesystem::path destination = folder / (name + m_NameExtension);

		const bool unchanged = renaming && destination.lexically_normal() == m_NameTarget.lexically_normal();
		const bool exists = !unchanged && !name.empty() && std::filesystem::exists(destination);
		const bool valid = IsValidFileName(name) && !exists;

		if (exists)
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Un element porte deja ce nom.");
		else if (!name.empty() && !IsValidFileName(name))
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Nom invalide (\\ / : * ? \" < > |).");

		ImGui::Spacing();
		confirm |= ImGui::Button(renaming ? "Renommer" : "Creer", ImVec2(120, 0));
		ImGui::SameLine();
		const bool cancel = ImGui::Button("Annuler", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape);

		if (confirm && (unchanged || valid))
		{
			if (!unchanged)
				ApplyName(name);
			m_NameMode = NamePopupMode::None;
			ImGui::CloseCurrentPopup();
		}
		else if (cancel)
		{
			m_NameMode = NamePopupMode::None;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	void ContentBrowserPanel::DrawDeletePopup()
	{
		const char* popupName = "Supprimer##ContentBrowserDeletePopup";

		if (m_OpenDeletePopup)
		{
			ImGui::OpenPopup(popupName);
			m_OpenDeletePopup = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		const std::string name = m_PendingDeletePath.filename().string();
		const bool isDirectory = std::filesystem::is_directory(m_PendingDeletePath);

		if (isDirectory)
		{
			ImGui::Text("Supprimer le dossier \"%s\" et tout son contenu ?", name.c_str());
			if (m_PendingDeleteCount > 0)
				ImGui::Text("Il contient %zu fichier(s).", m_PendingDeleteCount);
			else
				ImGui::TextDisabled("Le dossier est vide.");
		}
		else
		{
			ImGui::Text("Supprimer le fichier \"%s\" ?", name.c_str());
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Cette action est irreversible depuis l'editeur.");
		ImGui::TextDisabled("(Les fichiers sont envoyes dans la corbeille de Windows.)");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Bouton rouge : on ne supprime pas par erreur
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.10f, 0.10f, 1.0f));
		const bool confirm = ImGui::Button("Supprimer", ImVec2(120, 0));
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		const bool cancel = ImGui::Button("Annuler", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape);

		if (confirm)
			DeleteEntry(m_PendingDeletePath);

		if (confirm || cancel)
		{
			m_PendingDeletePath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	void ContentBrowserPanel::HandleShortcuts()
	{
		if (!HasSelection() || ImGui::GetIO().WantTextInput)
			return;
		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			return;

		if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
		{
			const bool isDirectory = std::filesystem::is_directory(m_Selected);
			OpenNamePopup(NamePopupMode::Rename,
				isDirectory ? m_Selected.filename().string() : m_Selected.stem().string(),
				isDirectory ? "" : m_Selected.extension().string(), m_Selected);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			RequestDelete(m_Selected);
		}
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		m_ThumbnailBudget = 2; // miniatures 3D à générer au maximum par frame

		// Sélection supprimée depuis l'explorateur Windows
		if (HasSelection() && !std::filesystem::exists(m_Selected))
			m_Selected.clear();

		DrawBreadcrumbs();
		HandleShortcuts();

		// ---------- Grille ----------
		const float padding = 16.0f;
		const float cellSize = m_ThumbnailSize + padding;
		int columnCount = (int)(ImGui::GetContentRegionAvail().x / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);
		for (const Entry& entry : ListDirectory())
		{
			DrawEntry(entry);
			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		// Clic dans le vide : désélectionne
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
			ClearSelection();

		// ---------- Zone vide sous la grille ----------
		// Permet de lâcher une entité de la hiérarchie dans le dossier courant (= créer un prefab)
		{
			const ImVec2 available = ImGui::GetContentRegionAvail();
			ImGui::InvisibleButton("##BlankArea", ImVec2((std::max)(available.x, 1.0f), (std::max)(available.y, 48.0f)));

			// C'est un élément ImGui : on refait à la main le clic gauche (désélection) et le clic droit (menu Creer)
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				ClearSelection();
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				ImGui::OpenPopup("##BlankContext");

			const ImGuiPayload* payload = ImGui::GetDragDropPayload();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && payload && payload->IsDataType("SCENE_ENTITY"))
			{
				ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), s_DropColor, 6.0f, 0, 2.0f);
			}
			AcceptMoveDrop(m_CurrentDirectory);
		}

		DrawBlankContextMenu();
		DrawNamePopup();
		DrawDeletePopup();

		// ---------- Actions différées (après le parcours du dossier) ----------
		if (m_HasPendingMove)
		{
			MoveEntry(m_PendingMoveSource, m_PendingMoveTarget);
			m_HasPendingMove = false;
		}

		if (m_HasPendingPrefab)
		{
			CreatePrefab(m_PendingPrefabEntity, m_PendingPrefabFolder);
			m_HasPendingPrefab = false;
		}

		if (m_HasPendingNavigate)
		{
			m_CurrentDirectory = m_PendingNavigate;
			m_HasPendingNavigate = false;
		}

		ImGui::End();
	}

	// =====================================================================
	//  Inspecteur de l'élément sélectionné
	// =====================================================================

	void ContentBrowserPanel::DrawSelectionInspector()
	{
		if (!HasSelection())
			return;

		const std::filesystem::path path = m_Selected;
		const bool isDirectory = std::filesystem::is_directory(path);
		const Entry entry{ path, isDirectory, GetAssetType(path, isDirectory) };

		// ---------- En-tête : miniature + nom + type ----------
		Ref<Texture2D> thumbnail = GetThumbnail(entry);
		const glm::vec4 tint = GetThumbnailTint(entry);
		ImGui::Image((ImTextureID)(uintptr_t)thumbnail->GetRendererID(), ImVec2(64, 64),
			ImVec2(0, 1), ImVec2(1, 0), ImVec4(tint.r, tint.g, tint.b, tint.a));

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Text("%s", path.filename().string().c_str());
		ImGui::TextDisabled("%s", GetTypeName(entry.Type));
		ImGui::TextDisabled("%s", Key(path).c_str());
		ImGui::EndGroup();

		ImGui::Separator();

		std::error_code error;

		// ---------- Contenu selon le type ----------
		switch (entry.Type)
		{
		case AssetType::Folder:
		{
			glm::vec3 color = GetFolderColor(path);
			if (ImGui::ColorEdit3("Couleur", glm::value_ptr(color)))
				SetFolderColor(path, color);

			size_t count = 0;
			for (auto& item : std::filesystem::directory_iterator(path, error))
				if (item.path().extension() != ".meta")
					count++;
			ImGui::Text("Contenu : %zu element(s)", count);
			break;
		}

		case AssetType::Material:
		{
			if (Ref<Material> material = Material::Load(Key(path)))
				EditorUI::MaterialEditor(*material);
			else
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Impossible de lire ce materiau.");
			break;
		}

		case AssetType::Texture:
		{
			Ref<Texture2D> texture = GetThumbnail(entry);
			const float width = (float)texture->GetWidth();
			const float height = (float)texture->GetHeight();
			ImGui::Text("Taille : %u x %u", texture->GetWidth(), texture->GetHeight());

			// Aperçu à la largeur de l'inspecteur
			const float previewWidth = ImGui::GetContentRegionAvail().x;
			if (width > 0.0f && previewWidth > 0.0f)
				ImGui::Image((ImTextureID)(uintptr_t)texture->GetRendererID(),
					ImVec2(previewWidth, previewWidth * height / width), ImVec2(0, 1), ImVec2(1, 0));
			break;
		}

		case AssetType::Mesh:
		{
			if (Ref<Mesh> mesh = Mesh::Create(Key(path)))
			{
				ImGui::Text("Triangles : %u", mesh->GetIndexCount() / 3);
				glm::vec3 size = mesh->GetBounds().Max - mesh->GetBounds().Min;
				ImGui::Text("Dimensions : %.2f x %.2f x %.2f m", size.x, size.y, size.z);
				ImGui::Text("Materiaux : %u", mesh->GetMaterialSlotCount());
				for (uint32_t i = 0; i < mesh->GetMaterialSlotCount(); i++)
					ImGui::BulletText("%u : %s", i, mesh->GetMaterialSlotName(i).c_str());
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Impossible de charger ce modele.");
			}
			break;
		}

		case AssetType::Prefab:
		{
			std::string rootName;
			size_t entityCount = 0;
			if (Prefab::ReadSummary(Key(path), rootName, entityCount))
			{
				ImGui::Text("Racine : %s", rootName.c_str());
				ImGui::Text("Entites : %zu", entityCount);
				ImGui::Spacing();
				if (ImGui::Button("Ajouter a la scene"))
					InstantiatePrefab(path);
				ImGui::TextDisabled("(ou glisser le prefab dans le viewport / la hierarchie)");
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Impossible de lire ce prefab.");
			}
			break;
		}

		case AssetType::Scene:
		{
			if (ImGui::Button("Ouvrir la scene") && m_Parent)
				m_Parent->RequestSceneAction(EditorLayer::PendingSceneAction::OpenPath, path);
			break;
		}

		default:
			break;
		}

		if (!isDirectory)
		{
			ImGui::Spacing();
			ImGui::TextDisabled("Taille du fichier : %s", FormatFileSize(std::filesystem::file_size(path, error)).c_str());
		}
	}
}
