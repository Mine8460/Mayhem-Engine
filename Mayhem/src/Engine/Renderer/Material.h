#pragma once

#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <string>
#include <cstdint>

namespace Mayhem
{
	// Textures d'un matériau. L'ordre = l'unité de texture utilisée par Standard.glsl (slot 0 = albedo…).
	enum class MaterialMap : uint32_t
	{
		Albedo = 0,   // couleur de base (sRGB)
		Normal,       // normal map en espace tangent (linéaire)
		Metallic,     // 0 = diélectrique, 1 = métal (linéaire, un canal)
		Roughness,    // 0 = lisse / miroir, 1 = rugueux (linéaire, un canal)
		Occlusion,    // occlusion ambiante (linéaire, un canal)
		Height,       // relief pour le parallax (linéaire, blanc = haut)
		Emission,     // couleur émise (sRGB)
		Count
	};

	// Canal d'une texture lue sur un seul canal (permet les textures "packées" ORM / glTF)
	enum class TextureChannel : int { R = 0, G, B, A };

	// Un matériau PBR (metallic / roughness) = un shader + ses paramètres. Sauvegardé en .mat (YAML).
	// Les champs sont publics pour être édités directement depuis l'éditeur :
	// changer un chemin (shader ou texture) recharge automatiquement la ressource.
	class Material
	{
	public:
		static constexpr uint32_t MapCount = (uint32_t)MaterialMap::Count;

		Material() = default;

		// ---------- Rendu ----------
		std::string m_ShaderPath = "assets/shaders/Standard.glsl";
		bool m_TwoSided = false;                   // false = l'arrière des faces n'est pas dessiné (back-face culling)
		bool m_Lit = true;                         // false = couleur brute (sprites, UI…)

		// ---------- Albedo ----------
		glm::vec4 m_AlbedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float m_AlphaCutoff = 0.01f;               // pixels sous ce seuil d'alpha = jetés

		// ---------- Normal ----------
		float m_NormalStrength = 1.0f;             // 0 = surface plate, 1 = normal map telle quelle, >1 = accentuée
		bool m_NormalFlipY = false;                // normal maps "DirectX" (Unreal, 3ds Max…) : canal vert inversé

		// ---------- Metallic / Roughness ----------
		float m_Metallic = 0.0f;                   // multiplié par la MetallicMap
		TextureChannel m_MetallicChannel = TextureChannel::R;
		float m_Roughness = 0.5f;                  // multiplié par la RoughnessMap
		TextureChannel m_RoughnessChannel = TextureChannel::R;
		bool m_RoughnessIsSmoothness = false;      // la map est une smoothness / glossiness (Unity) : on l'inverse

		// ---------- Occlusion ----------
		float m_OcclusionStrength = 1.0f;          // 0 = ignorée, 1 = pleine
		TextureChannel m_OcclusionChannel = TextureChannel::R;

		// ---------- Height (parallax occlusion mapping) ----------
		float m_HeightScale = 0.03f;               // profondeur du relief, en unités d'UV
		TextureChannel m_HeightChannel = TextureChannel::R;

		// ---------- Emission ----------
		glm::vec3 m_EmissionColor = { 0.0f, 0.0f, 0.0f };
		float m_EmissionIntensity = 0.0f;

		// ---------- UV ----------
		glm::vec2 m_Tiling = { 1.0f, 1.0f };
		glm::vec2 m_Offset = { 0.0f, 0.0f };

		// ---------- Textures ----------
		// Chemin vide = pas de texture (valeur neutre utilisée à la place)
		std::string& MapPath(MaterialMap _map) { return m_MapPaths[(uint32_t)_map]; }
		const std::string& MapPath(MaterialMap _map) const { return m_MapPaths[(uint32_t)_map]; }
		bool HasMap(MaterialMap _map) const { return !m_MapPaths[(uint32_t)_map].empty(); }

		// Texture chargée à la demande (mise en cache). nullptr si pas de chemin ou fichier introuvable.
		const Ref<Texture2D>& GetMap(MaterialMap _map);
		const Ref<Texture2D>& GetAlbedoMap() { return GetMap(MaterialMap::Albedo); }

		// "Albedo", "Normal"… (clé YAML "<Nom>Map" et libellé dans l'éditeur)
		static const char* GetMapName(MaterialMap _map);

		const Ref<Shader>& GetShader();

		const std::string& GetPath() const { return m_Path; }

		// Augmente à chaque sauvegarde : permet de savoir quand régénérer une miniature
		uint32_t GetRevision() const { return m_Revision; }

		// ---------- Fichiers .mat ----------
		static Ref<Material> Load(const std::string& _path); // en cache : même chemin = même Material
		bool Save(const std::string& _path);
		bool Save() { return !m_Path.empty() && Save(m_Path); }

		// Crée un nouveau .mat avec les valeurs par défaut et le renvoie
		static Ref<Material> CreateAsset(const std::string& _path);

		// ---------- Matériaux par défaut ----------
		static const Ref<Material>& GetDefault();      // éclairé (meshes)
		static const Ref<Material>& GetDefaultUnlit(); // non éclairé (sprites / quads)

	private:
		std::string m_Path;
		uint32_t m_Revision = 0;

		Ref<Shader> m_Shader;
		std::string m_LoadedShaderPath;

		std::string m_MapPaths[MapCount];
		Ref<Texture2D> m_Maps[MapCount];
		std::string m_LoadedMapPaths[MapCount];
	};
}
