#include "Enpch.h"
#include "Material.h"
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <Engine/Core/AssetsManager.h>

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>

namespace Mayhem
{
	// ---------- Caches de ressources partagées ----------

	// Clé de cache : l'identifiant de l'asset (un fichier renommé / déplacé garde la même entrée)
	static std::string CacheKey(const std::string& _path)
	{
		AssetID id = AssetsManager::GetID(_path);
		return id != 0 ? "#" + std::to_string(id) : AssetsManager::Normalize(_path);
	}

	static Ref<Shader> LoadShaderCached(const std::string& _rawPath)
	{
		static std::unordered_map<std::string, Ref<Shader>> s_Cache;

		if (_rawPath.empty())
			return nullptr;

		const std::string path = AssetsManager::Resolve(_rawPath);
		const std::string key = CacheKey(path);

		auto it = s_Cache.find(key);
		if (it != s_Cache.end())
			return it->second;

		if (!std::filesystem::exists(path))
		{
			std::cerr << "[Material] Shader introuvable : " << path << std::endl;
			return nullptr;
		}

		Ref<Shader> shader = Shader::Create(path);
		s_Cache[key] = shader;
		return shader;
	}

	static Ref<Texture2D> LoadTextureCached(const std::string& _rawPath)
	{
		static std::unordered_map<std::string, Ref<Texture2D>> s_Cache;

		if (_rawPath.empty())
			return nullptr;

		const std::string path = AssetsManager::Resolve(_rawPath);
		const std::string key = CacheKey(path);

		auto it = s_Cache.find(key);
		if (it != s_Cache.end())
			return it->second;

		if (!std::filesystem::exists(path))
		{
			std::cerr << "[Material] Texture introuvable : " << path << std::endl;
			return nullptr;
		}

		Ref<Texture2D> texture = Texture2D::Create(path);
		s_Cache[key] = texture;
		return texture;
	}

	// Lit une référence d'asset : identifiant en priorité, chemin en secours (anciens fichiers)
	static std::string ReadAssetReference(const YAML::Node& _node, const char* _idKey, const char* _pathKey, const std::string& _default)
	{
		if (_node[_idKey])
		{
			std::string path = AssetsManager::GetPath(_node[_idKey].as<uint64_t>());
			if (!path.empty())
				return path;
		}
		if (_node[_pathKey])
			return AssetsManager::Resolve(_node[_pathKey].as<std::string>());
		return _default;
	}

	// ---------- Ressources ----------

	const Ref<Shader>& Material::GetShader()
	{
		if (m_LoadedShaderPath != m_ShaderPath)
		{
			m_Shader = LoadShaderCached(m_ShaderPath);
			m_LoadedShaderPath = m_ShaderPath;
		}
		return m_Shader;
	}

	const Ref<Texture2D>& Material::GetMap(MaterialMap _map)
	{
		const uint32_t i = (uint32_t)_map;
		if (m_LoadedMapPaths[i] != m_MapPaths[i])
		{
			m_Maps[i] = LoadTextureCached(m_MapPaths[i]);
			m_LoadedMapPaths[i] = m_MapPaths[i];
		}
		return m_Maps[i];
	}

	const char* Material::GetMapName(MaterialMap _map)
	{
		switch (_map)
		{
		case MaterialMap::Albedo:    return "Albedo";
		case MaterialMap::Normal:    return "Normal";
		case MaterialMap::Metallic:  return "Metallic";
		case MaterialMap::Roughness: return "Roughness";
		case MaterialMap::Occlusion: return "Occlusion";
		case MaterialMap::Height:    return "Height";
		case MaterialMap::Emission:  return "Emission";
		default:                     return "Unknown";
		}
	}

	// ---------- Helpers YAML (noms uniques pour ne pas entrer en conflit avec ton SceneSerializer) ----------

	static float ReadFloat(const YAML::Node& _n, float _def)
	{
		return _n ? _n.as<float>() : _def;
	}

	static bool ReadBool(const YAML::Node& _n, bool _def)
	{
		return _n ? _n.as<bool>() : _def;
	}

	static TextureChannel ReadChannel(const YAML::Node& _n, TextureChannel _def)
	{
		if (!_n) return _def;
		int c = _n.as<int>();
		return (c >= 0 && c <= 3) ? (TextureChannel)c : _def;
	}

	static glm::vec2 ReadVec2(const YAML::Node& _n, const glm::vec2& _def)
	{
		if (!_n || !_n.IsSequence() || _n.size() != 2) return _def;
		return { _n[0].as<float>(), _n[1].as<float>() };
	}

	static glm::vec3 ReadVec3(const YAML::Node& _n, const glm::vec3& _def)
	{
		if (!_n || !_n.IsSequence() || _n.size() != 3) return _def;
		return { _n[0].as<float>(), _n[1].as<float>(), _n[2].as<float>() };
	}

	static glm::vec4 ReadVec4(const YAML::Node& _n, const glm::vec4& _def)
	{
		if (!_n || !_n.IsSequence() || _n.size() != 4) return _def;
		return { _n[0].as<float>(), _n[1].as<float>(), _n[2].as<float>(), _n[3].as<float>() };
	}

	static void WriteVec2(YAML::Emitter& _out, const char* _key, const glm::vec2& _v)
	{
		_out << YAML::Key << _key << YAML::Value << YAML::Flow << YAML::BeginSeq << _v.x << _v.y << YAML::EndSeq;
	}

	static void WriteVec3(YAML::Emitter& _out, const char* _key, const glm::vec3& _v)
	{
		_out << YAML::Key << _key << YAML::Value << YAML::Flow << YAML::BeginSeq << _v.x << _v.y << _v.z << YAML::EndSeq;
	}

	static void WriteVec4(YAML::Emitter& _out, const char* _key, const glm::vec4& _v)
	{
		_out << YAML::Key << _key << YAML::Value << YAML::Flow << YAML::BeginSeq << _v.x << _v.y << _v.z << _v.w << YAML::EndSeq;
	}

	// ---------- Chargement / sauvegarde ----------

	Ref<Material> Material::Load(const std::string& _rawPath)
	{
		static std::unordered_map<std::string, Ref<Material>> s_Cache;

		// Chemin actuel (le fichier a pu être renommé / déplacé) et clé = identifiant de l'asset :
		// "assets\\a.mat", "assets/a.mat" ou un ancien nom donnent le même matériau
		const std::string _path = AssetsManager::Resolve(_rawPath);
		const std::string key = CacheKey(_path);

		auto it = s_Cache.find(key);
		if (it != s_Cache.end())
			return it->second;

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(_path);
		}
		catch (const YAML::Exception& e)
		{
			std::cerr << "[Material] Impossible de lire '" << _path << "' : " << e.what() << std::endl;
			return nullptr;
		}

		YAML::Node node = data["Material"];
		if (!node)
		{
			std::cerr << "[Material] '" << _path << "' n'est pas un fichier .mat valide" << std::endl;
			return nullptr;
		}

		Ref<Material> mat(new Material());
		mat->m_Path = _path;

		mat->m_ShaderPath        = ReadAssetReference(node, "ShaderID", "Shader", mat->m_ShaderPath);
		mat->m_TwoSided          = ReadBool(node["TwoSided"], mat->m_TwoSided);
		mat->m_Lit               = ReadBool(node["Lit"], mat->m_Lit);

		for (uint32_t i = 0; i < MapCount; i++)
		{
			const std::string name = std::string(GetMapName((MaterialMap)i)) + "Map";
			mat->m_MapPaths[i] = ReadAssetReference(node, (name + "ID").c_str(), name.c_str(), "");
		}

		mat->m_AlbedoColor       = ReadVec4(node["AlbedoColor"], mat->m_AlbedoColor);
		mat->m_AlphaCutoff       = ReadFloat(node["AlphaCutoff"], mat->m_AlphaCutoff);

		mat->m_NormalStrength    = ReadFloat(node["NormalStrength"], mat->m_NormalStrength);
		mat->m_NormalFlipY       = ReadBool(node["NormalFlipY"], mat->m_NormalFlipY);

		mat->m_Metallic          = ReadFloat(node["Metallic"], mat->m_Metallic);
		mat->m_MetallicChannel   = ReadChannel(node["MetallicChannel"], mat->m_MetallicChannel);
		mat->m_Roughness         = ReadFloat(node["Roughness"], mat->m_Roughness);
		mat->m_RoughnessChannel  = ReadChannel(node["RoughnessChannel"], mat->m_RoughnessChannel);
		mat->m_RoughnessIsSmoothness = ReadBool(node["RoughnessIsSmoothness"], mat->m_RoughnessIsSmoothness);

		// Anciens .mat (Blinn-Phong) : on convertit la "Shininess" en rugosité équivalente
		if (!node["Roughness"] && node["Shininess"])
		{
			const float shininess = glm::max(node["Shininess"].as<float>(), 1.0f);
			const float alpha = std::sqrt(2.0f / (shininess + 2.0f)); // exposant de Blinn -> alpha GGX
			mat->m_Roughness = glm::clamp(std::sqrt(alpha), 0.05f, 1.0f);
		}

		mat->m_OcclusionStrength = ReadFloat(node["OcclusionStrength"], mat->m_OcclusionStrength);
		mat->m_OcclusionChannel  = ReadChannel(node["OcclusionChannel"], mat->m_OcclusionChannel);

		mat->m_HeightScale       = ReadFloat(node["HeightScale"], mat->m_HeightScale);
		mat->m_HeightChannel     = ReadChannel(node["HeightChannel"], mat->m_HeightChannel);

		mat->m_EmissionColor     = ReadVec3(node["EmissionColor"], mat->m_EmissionColor);
		mat->m_EmissionIntensity = ReadFloat(node["EmissionIntensity"], mat->m_EmissionIntensity);

		mat->m_Tiling            = ReadVec2(node["Tiling"], mat->m_Tiling);
		mat->m_Offset            = ReadVec2(node["Offset"], mat->m_Offset);

		s_Cache[key] = mat;
		return mat;
	}

	bool Material::Save(const std::string& _rawPath)
	{
		// Si le .mat a été renommé / déplacé depuis son chargement, on écrit au nouvel endroit
		const std::string _path = AssetsManager::Resolve(_rawPath);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

		// Références d'assets : chemin (lisible) + identifiant (survit aux renommages / déplacements)
		const std::string shaderPath = AssetsManager::Resolve(m_ShaderPath);
		out << YAML::Key << "Shader" << YAML::Value << shaderPath;
		out << YAML::Key << "ShaderID" << YAML::Value << AssetsManager::GetID(shaderPath);
		out << YAML::Key << "TwoSided" << YAML::Value << m_TwoSided;
		out << YAML::Key << "Lit" << YAML::Value << m_Lit;

		for (uint32_t i = 0; i < MapCount; i++)
		{
			const std::string name = std::string(GetMapName((MaterialMap)i)) + "Map";
			const std::string mapPath = AssetsManager::Resolve(m_MapPaths[i]);
			out << YAML::Key << name << YAML::Value << mapPath;
			out << YAML::Key << (name + "ID") << YAML::Value << AssetsManager::GetID(mapPath);
		}

		WriteVec4(out, "AlbedoColor", m_AlbedoColor);
		out << YAML::Key << "AlphaCutoff" << YAML::Value << m_AlphaCutoff;

		out << YAML::Key << "NormalStrength" << YAML::Value << m_NormalStrength;
		out << YAML::Key << "NormalFlipY" << YAML::Value << m_NormalFlipY;

		out << YAML::Key << "Metallic" << YAML::Value << m_Metallic;
		out << YAML::Key << "MetallicChannel" << YAML::Value << (int)m_MetallicChannel;
		out << YAML::Key << "Roughness" << YAML::Value << m_Roughness;
		out << YAML::Key << "RoughnessChannel" << YAML::Value << (int)m_RoughnessChannel;
		out << YAML::Key << "RoughnessIsSmoothness" << YAML::Value << m_RoughnessIsSmoothness;

		out << YAML::Key << "OcclusionStrength" << YAML::Value << m_OcclusionStrength;
		out << YAML::Key << "OcclusionChannel" << YAML::Value << (int)m_OcclusionChannel;

		out << YAML::Key << "HeightScale" << YAML::Value << m_HeightScale;
		out << YAML::Key << "HeightChannel" << YAML::Value << (int)m_HeightChannel;

		WriteVec3(out, "EmissionColor", m_EmissionColor);
		out << YAML::Key << "EmissionIntensity" << YAML::Value << m_EmissionIntensity;

		WriteVec2(out, "Tiling", m_Tiling);
		WriteVec2(out, "Offset", m_Offset);

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream file(_path);
		if (!file)
		{
			std::cerr << "[Material] Impossible d'écrire '" << _path << "'" << std::endl;
			return false;
		}

		file << out.c_str();
		m_Path = _path;
		m_Revision++;
		return true;
	}

	Ref<Material> Material::CreateAsset(const std::string& _path)
	{
		Ref<Material> mat(new Material());
		if (!mat->Save(_path))
			return nullptr;
		return Load(_path);
	}

	// ---------- Défauts ----------

	const Ref<Material>& Material::GetDefault()
	{
		static Ref<Material> s_Default(new Material());
		return s_Default;
	}

	const Ref<Material>& Material::GetDefaultUnlit()
	{
		static Ref<Material> s_DefaultUnlit = []()
		{
			Ref<Material> mat(new Material());
			mat->m_Lit = false;
			mat->m_TwoSided = true; // un sprite reste visible quand on le regarde de dos
			return mat;
		}();
		return s_DefaultUnlit;
	}
}
