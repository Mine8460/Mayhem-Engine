#include "EditorWidgets.h"

#include <Engine/Core/AssetsManager.h>
#include "Panels/ContentBrowserPanel.h"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <cstring>
#include <cctype>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace Mayhem::EditorUI
{
	static std::string ToLower(std::string _s)
	{
		for (auto& c : _s) c = (char)std::tolower((unsigned char)c);
		return _s;
	}

	// =====================================================================
	//  Sélecteur d'assets (liste de tous les fichiers d'un type dans le projet)
	// =====================================================================

	struct PickerItem
	{
		std::string Path;    // "assets/textures/brique.png"
		std::string Name;    // "brique.png"
		std::string Folder;  // "assets/textures"
		bool IsTexture = false;
	};

	static std::vector<PickerItem> s_PickerItems;
	static char s_PickerFilter[64] = "";

	static void RefreshPicker(std::initializer_list<const char*> _extensions)
	{
		s_PickerItems.clear();

		std::error_code error;
		for (auto& entry : std::filesystem::recursive_directory_iterator("assets", error))
		{
			if (!entry.is_regular_file())
				continue;

			const std::string ext = ToLower(entry.path().extension().string());
			bool accepted = false;
			for (const char* allowed : _extensions)
				accepted |= (ext == allowed);
			if (!accepted)
				continue;

			PickerItem item;
			item.Path = entry.path().lexically_normal().generic_string();
			item.Name = entry.path().filename().string();
			item.Folder = entry.path().parent_path().lexically_normal().generic_string();
			item.IsTexture = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga");
			s_PickerItems.push_back(item);
		}

		std::sort(s_PickerItems.begin(), s_PickerItems.end(),
			[](const PickerItem& a, const PickerItem& b) { return ToLower(a.Name) < ToLower(b.Name); });
	}

	static Ref<Texture2D> GetPickerThumbnail(const std::string& _path)
	{
		// Même cache que le reste du moteur : pas de deuxième copie de la texture en mémoire
		return AssetsManager::GetInstance()->GetTexture(_path);
	}

	// Contenu du popup. Renvoie Dropped (choix dans _outPath), Cleared ("None") ou None.
	static AssetFieldResult DrawAssetPicker(const std::string& _currentPath, std::string& _outPath)
	{
		AssetFieldResult result = AssetFieldResult::None;

		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##Search", "Rechercher...", s_PickerFilter, sizeof(s_PickerFilter));
		ImGui::Separator();

		ImGui::BeginChild("##PickerList", ImVec2(0, 0), false);

		if (ImGui::Selectable("None", _currentPath.empty()))
		{
			result = AssetFieldResult::Cleared;
			ImGui::CloseCurrentPopup();
		}

		const std::string filter = ToLower(s_PickerFilter);
		const float rowHeight = 36.0f;

		for (const PickerItem& item : s_PickerItems)
		{
			if (!filter.empty() && ToLower(item.Name).find(filter) == std::string::npos)
				continue;

			ImGui::PushID(item.Path.c_str());

			const ImVec2 start = ImGui::GetCursorPos();
			if (ImGui::Selectable("##Item", item.Path == _currentPath, 0, ImVec2(0, rowHeight)))
			{
				_outPath = item.Path;
				result = AssetFieldResult::Dropped;
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", item.Path.c_str());
			const ImVec2 end = ImGui::GetCursorPos();

			// Miniature (textures) + nom + dossier, dessinés par-dessus la ligne sélectionnable
			float textX = start.x + 4.0f;
			if (item.IsTexture)
			{
				if (Ref<Texture2D> thumbnail = GetPickerThumbnail(item.Path))
				{
					ImGui::SetCursorPos(ImVec2(start.x + 2.0f, start.y + 2.0f));
					ImGui::Image((ImTextureID)(uintptr_t)thumbnail->GetRendererID(), ImVec2(rowHeight - 4.0f, rowHeight - 4.0f), ImVec2(0, 1), ImVec2(1, 0));
				}
				textX = start.x + rowHeight + 4.0f;
			}

			ImGui::SetCursorPos(ImVec2(textX, start.y + 1.0f));
			ImGui::Text("%s", item.Name.c_str());
			ImGui::SetCursorPos(ImVec2(textX, start.y + rowHeight * 0.5f));
			ImGui::TextDisabled("%s", item.Folder.c_str());

			ImGui::SetCursorPos(end);
			ImGui::PopID();
		}

		ImGui::EndChild();
		return result;
	}

	// =====================================================================

	void SectionTitle(const char* _text)
	{
		ImGui::Spacing();
		ImGui::TextDisabled("%s", _text);
		ImGui::Separator();
	}

	AssetFieldResult AssetField(const char* _label, const std::string& _displayName, bool _hasValue,
		std::initializer_list<const char*> _extensions, std::string& _outPath, const std::string& _tooltip)
	{
		AssetFieldResult result = AssetFieldResult::None;
		ImGui::PushID(_label);

		if (_hasValue)
		{
			if (ImGui::Button("X"))
				result = AssetFieldResult::Cleared;
			ImGui::SameLine();
		}

		// Bouton rond (comme Unity) : ouvre la liste des assets compatibles
		bool openPicker = ImGui::Button("o");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Choisir dans le projet");
		ImGui::SameLine();

		char buffer[256];
		strncpy_s(buffer, sizeof(buffer), _displayName.c_str(), _TRUNCATE);
		ImGui::InputText(_label, buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
		openPicker |= ImGui::IsItemClicked(); // cliquer sur le champ ouvre aussi la liste

		if (!_tooltip.empty() && ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", _tooltip.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				std::filesystem::path path = (const wchar_t*)payload->Data;
				std::string ext = ToLower(path.extension().string());

				for (const char* allowed : _extensions)
				{
					if (ext == allowed)
					{
						_outPath = path.lexically_normal().generic_string();
						result = AssetFieldResult::Dropped;
						break;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// ---------- Sélecteur ----------
		if (openPicker)
		{
			RefreshPicker(_extensions);
			s_PickerFilter[0] = '\0';
			ImGui::OpenPopup("##AssetPicker");
		}

		ImGui::SetNextWindowSize(ImVec2(380.0f, 420.0f), ImGuiCond_Appearing);
		if (ImGui::BeginPopup("##AssetPicker"))
		{
			// _tooltip contient le chemin actuel quand il y en a un : il est surligné dans la liste
			AssetFieldResult picked = DrawAssetPicker(_hasValue ? _tooltip : "", _outPath);
			if (picked != AssetFieldResult::None)
				result = picked;
			ImGui::EndPopup();
		}

		ImGui::PopID();
		return result;
	}

	// ---------- Champs de textures du matériau ----------

	static const std::initializer_list<const char*> s_TextureExtensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

	// Miniature + champ d'asset pour une map du matériau. Renvoie true si la map a changé.
	static bool TextureMapField(Material& _mat, MaterialMap _map, const char* _label)
	{
		bool changed = false;
		std::string& path = _mat.MapPath(_map);

		ImGui::PushID(_label);

		// Miniature (survol = aperçu plus grand)
		const float size = ImGui::GetFrameHeight();
		if (const Ref<Texture2D>& texture = _mat.GetMap(_map))
		{
			ImGui::Image((ImTextureID)(uintptr_t)texture->GetRendererID(), ImVec2(size, size), ImVec2(0, 1), ImVec2(1, 0));
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Image((ImTextureID)(uintptr_t)texture->GetRendererID(), ImVec2(160.0f, 160.0f), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::Text("%u x %u", texture->GetWidth(), texture->GetHeight());
				ImGui::EndTooltip();
			}
		}
		else
		{
			ImGui::Dummy(ImVec2(size, size));
		}
		ImGui::SameLine();

		std::string dropped;
		const std::string name = path.empty() ? "None" : std::filesystem::path(path).filename().string();
		switch (AssetField(_label, name, !path.empty(), s_TextureExtensions, dropped, path))
		{
		case AssetFieldResult::Dropped: path = dropped; changed = true; break;
		case AssetFieldResult::Cleared: path.clear(); changed = true; break;
		default: break;
		}

		ImGui::PopID();
		return changed;
	}

	// Choix du canal lu dans une texture "packée" (ORM, glTF metallicRoughness…)
	static bool ChannelCombo(const char* _label, TextureChannel& _channel)
	{
		static const char* s_Names[] = { "R", "G", "B", "A" };
		int value = (int)_channel;
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("AAAA").x + ImGui::GetFrameHeight() * 1.5f);
		if (ImGui::Combo(_label, &value, s_Names, 4))
		{
			_channel = (TextureChannel)value;
			return true;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Canal lu dans la texture (textures packees : ORM, glTF...)");
		return false;
	}

	void MaterialEditor(Material& _material)
	{
		Material& mat = _material;
		bool save = false;

		// Sauvegarde quand l'utilisateur termine une modification (relâche le slider, valide…)
		auto saveIfEdited = [&save]() { if (ImGui::IsItemDeactivatedAfterEdit()) save = true; };

		ImGui::PushID("MaterialEditor");

		// ---------- Rendu ----------
		SectionTitle("Rendu");
		std::string dropped;
		std::string shaderName = std::filesystem::path(mat.m_ShaderPath).filename().string();
		switch (AssetField("Shader", shaderName, !mat.m_ShaderPath.empty(), { ".glsl" }, dropped, mat.m_ShaderPath))
		{
		case AssetFieldResult::Dropped: mat.m_ShaderPath = dropped; save = true; break;
		case AssetFieldResult::Cleared: mat.m_ShaderPath = Material().m_ShaderPath; save = true; break; // revient au Standard
		default: break;
		}

		if (ImGui::Checkbox("Lit", &mat.m_Lit))
			save = true;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Decoche = couleur brute, sans lumiere (sprites, UI...)");
		ImGui::SameLine();
		if (ImGui::Checkbox("Two-Sided", &mat.m_TwoSided))
			save = true;
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Affiche aussi l'arriere des faces (feuillages, tissus, plans fins...)");

		// ---------- Albedo ----------
		SectionTitle("Albedo");
		save |= TextureMapField(mat, MaterialMap::Albedo, "Albedo Map");
		ImGui::ColorEdit4("Albedo", glm::value_ptr(mat.m_AlbedoColor));
		saveIfEdited();
		ImGui::SliderFloat("Alpha Cutoff", &mat.m_AlphaCutoff, 0.0f, 1.0f);
		saveIfEdited();

		if (mat.m_Lit)
		{
			// ---------- Metallic / Roughness ----------
			SectionTitle("Metallic / Roughness");
			save |= TextureMapField(mat, MaterialMap::Metallic, "Metallic Map");
			if (mat.HasMap(MaterialMap::Metallic))
			{
				ImGui::Indent();
				save |= ChannelCombo("Canal##Metallic", mat.m_MetallicChannel);
				ImGui::Unindent();
			}
			ImGui::SliderFloat("Metallic", &mat.m_Metallic, 0.0f, 1.0f);
			saveIfEdited();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("0 = plastique, bois, pierre... / 1 = metal (multiplie la Metallic Map)");

			save |= TextureMapField(mat, MaterialMap::Roughness, "Roughness Map");
			if (mat.HasMap(MaterialMap::Roughness))
			{
				ImGui::Indent();
				save |= ChannelCombo("Canal##Roughness", mat.m_RoughnessChannel);
				ImGui::SameLine();
				if (ImGui::Checkbox("Smoothness map", &mat.m_RoughnessIsSmoothness))
					save = true;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("La texture est une smoothness / glossiness (blanc = lisse) : elle est inversee");
				ImGui::Unindent();
			}
			ImGui::SliderFloat("Roughness", &mat.m_Roughness, 0.0f, 1.0f);
			saveIfEdited();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("0 = miroir / 1 = completement mat (multiplie la Roughness Map)");

			// ---------- Normal ----------
			SectionTitle("Normal");
			save |= TextureMapField(mat, MaterialMap::Normal, "Normal Map");
			if (mat.HasMap(MaterialMap::Normal))
			{
				ImGui::DragFloat("Normal Strength", &mat.m_NormalStrength, 0.01f, 0.0f, 5.0f);
				saveIfEdited();
				if (ImGui::Checkbox("Flip Y (DirectX)", &mat.m_NormalFlipY))
					save = true;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("A cocher si le relief semble inverse (normal maps Unreal / 3ds Max / Substance DirectX)");
			}

			// ---------- Height ----------
			SectionTitle("Height (parallax)");
			save |= TextureMapField(mat, MaterialMap::Height, "Height Map");
			if (mat.HasMap(MaterialMap::Height))
			{
				ImGui::Indent();
				save |= ChannelCombo("Canal##Height", mat.m_HeightChannel);
				ImGui::Unindent();
				ImGui::SliderFloat("Height Scale", &mat.m_HeightScale, 0.0f, 0.15f, "%.3f");
				saveIfEdited();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Profondeur du relief (blanc = haut, noir = creux)");
			}

			// ---------- Occlusion ----------
			SectionTitle("Ambient Occlusion");
			save |= TextureMapField(mat, MaterialMap::Occlusion, "Occlusion Map");
			if (mat.HasMap(MaterialMap::Occlusion))
			{
				ImGui::Indent();
				save |= ChannelCombo("Canal##Occlusion", mat.m_OcclusionChannel);
				ImGui::Unindent();
				ImGui::SliderFloat("Occlusion Strength", &mat.m_OcclusionStrength, 0.0f, 1.0f);
				saveIfEdited();
			}
		}

		// ---------- Emission ----------
		SectionTitle("Emission");
		save |= TextureMapField(mat, MaterialMap::Emission, "Emission Map");
		ImGui::ColorEdit3("Emission", glm::value_ptr(mat.m_EmissionColor));
		saveIfEdited();
		ImGui::DragFloat("Emission Intensity", &mat.m_EmissionIntensity, 0.05f, 0.0f, 100.0f);
		saveIfEdited();
		if (mat.HasMap(MaterialMap::Emission) && mat.m_EmissionIntensity <= 0.0f && ImGui::IsItemHovered())
			ImGui::SetTooltip("L'Emission Map est multipliee par la couleur et l'intensite");

		// ---------- UV ----------
		SectionTitle("UV");
		ImGui::DragFloat2("Tiling", glm::value_ptr(mat.m_Tiling), 0.05f);
		saveIfEdited();
		ImGui::DragFloat2("Offset", glm::value_ptr(mat.m_Offset), 0.01f);
		saveIfEdited();

		ImGui::PopID();

		if (save)
			mat.Save();
	}

	// =====================================================================
	//  Prefabs
	// =====================================================================

	glm::vec4 GetPrefabColor()
	{
		return { 0.45f, 0.72f, 1.0f, 1.0f }; // bleu clair (comme Unity) : se distingue du violet de sélection
	}

	Entity PrefabInstanceBar(Entity _entity, Scene& _scene, ContentBrowserPanel* _contentBrowser)
	{
		Entity root = Prefab::FindInstanceRoot(_entity);
		if (!root)
			return _entity;

		Entity result = _entity;
		const glm::vec4 color = GetPrefabColor();
		const ImVec4 prefabColor(color.r, color.g, color.b, color.a);
		const std::string path = Prefab::GetPrefabPath(root);

		ImGui::PushID("PrefabInstanceBar");
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(prefabColor.x * 0.18f, prefabColor.y * 0.18f, prefabColor.z * 0.25f, 1.0f));
		const float height = ImGui::GetFrameHeightWithSpacing() * (_entity != root ? 3.0f : 2.0f) + ImGui::GetStyle().WindowPadding.y * 2.0f;
		ImGui::BeginChild("##PrefabBar", ImVec2(0, height), true);

		// ---------- Nom du prefab ----------
		if (path.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Prefab introuvable (supprime ?)");
		}
		else
		{
			ImGui::TextColored(prefabColor, "Prefab : %s", std::filesystem::path(path).stem().string().c_str());
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", path.c_str());
		}

		// Enfant d'une instance : les boutons agissent sur la racine
		if (_entity != root)
		{
			ImGui::TextDisabled("Fait partie de \"%s\"", root.GetComponent<TagComponent>().Tag.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Selectionner la racine"))
				result = root;
		}

		// ---------- Actions ----------
		if (!path.empty())
		{
			if (ImGui::Button("Voir l'asset") && _contentBrowser)
				_contentBrowser->Select(path); // l'inspecteur montre alors le prefab
			ImGui::SameLine();

			if (ImGui::Button("Appliquer"))
			{
				const uint64_t uuid = (uint64_t)root.GetUUID();
				const bool rootSelected = (_entity == root);
				const uint64_t selectedUUID = (uint64_t)_entity.GetUUID();
				Prefab::Apply(_scene, root);

				// L'entité appliquée n'est pas recréée : la sélection reste valide
				result = _scene.GetEntityByUUID(rootSelected ? uuid : selectedUUID);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Enregistre cette instance dans le prefab et met a jour les autres instances de la scene");
			ImGui::SameLine();

			if (ImGui::Button("Retablir"))
				result = Prefab::Revert(_scene, root); // les enfants sont recréés : on sélectionne la racine
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Annule les modifications de cette instance (garde son nom et sa position)");
			ImGui::SameLine();
		}

		if (ImGui::Button("Detacher"))
			Prefab::Unpack(root);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Coupe le lien avec le prefab : l'entite redevient une entite normale");

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopID();

		return result;
	}
}
