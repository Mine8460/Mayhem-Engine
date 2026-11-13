#pragma once

#include <Engine.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Scene/Prefab.h>

#include <string>
#include <initializer_list>

namespace Mayhem { class ContentBrowserPanel; }

// Widgets ImGui partagés par les panneaux de l'éditeur
namespace Mayhem::EditorUI
{
	enum class AssetFieldResult { None, Dropped, Cleared };

	// Petit titre de section (remplace ImGui::SeparatorText, absent avant ImGui 1.89.4)
	void SectionTitle(const char* _text);

	// Champ en lecture seule + croix pour vider + dépôt depuis le Content Browser filtré par extension.
	// _outPath reçoit le chemin déposé quand le résultat vaut Dropped.
	AssetFieldResult AssetField(const char* _label, const std::string& _displayName, bool _hasValue,
		std::initializer_list<const char*> _extensions, std::string& _outPath, const std::string& _tooltip = "");

	// Toutes les propriétés d'un matériau. Sauvegarde automatique du .mat à chaque modification.
	void MaterialEditor(Material& _material);

	// Couleur des entités qui font partie d'un prefab (hiérarchie, inspecteur)
	glm::vec4 GetPrefabColor();

	// Bandeau "Prefab" en haut de l'inspecteur quand _entity fait partie d'une instance :
	// Voir l'asset / Appliquer / Retablir / Detacher.
	// Renvoie l'entité à sélectionner ensuite (différente de _entity après Retablir ou Appliquer).
	Entity PrefabInstanceBar(Entity _entity, Scene& _scene, ContentBrowserPanel* _contentBrowser);
}
