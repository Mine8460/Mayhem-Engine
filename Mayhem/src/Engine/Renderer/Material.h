#pragma once

#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <string>

namespace Mayhem
{
	// Un matériau = un shader + ses paramètres. Sauvegardé en .mat (YAML).
	// Les champs sont publics pour être édités directement depuis l'éditeur :
	// changer m_ShaderPath ou m_AlbedoMapPath recharge automatiquement la ressource.
	class Material
	{
	public:
		Material() = default;

		// ---------- Rendu ----------
		std::string m_ShaderPath = "assets/shaders/Standard.glsl";

		// ---------- Surface ----------
		glm::vec4 m_AlbedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::string m_AlbedoMapPath;               // vide = pas de texture (blanc)
		glm::vec2 m_Tiling = { 1.0f, 1.0f };
		glm::vec2 m_Offset = { 0.0f, 0.0f };
		float m_AlphaCutoff = 0.01f;               // pixels sous ce seuil d'alpha = jetés

		// ---------- Éclairage ----------
		bool m_Lit = true;                         // false = couleur brute (sprites, UI…)
		float m_SpecularStrength = 0.0f;           // 0 = mat, 1 = très brillant
		float m_Shininess = 32.0f;                 // taille du reflet (grand = petit reflet net)
		glm::vec3 m_EmissionColor = { 0.0f, 0.0f, 0.0f };
		float m_EmissionIntensity = 0.0f;

		// Ressources (chargées à la demande, mises en cache)
		const Ref<Shader>& GetShader();
		const Ref<Texture2D>& GetAlbedoMap();

		const std::string& GetPath() const { return m_Path; }

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

		Ref<Shader> m_Shader;
		std::string m_LoadedShaderPath;

		Ref<Texture2D> m_AlbedoMap;
		std::string m_LoadedAlbedoMapPath;
	};
}
