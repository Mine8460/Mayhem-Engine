#include "Enpch.h"
#include "Material.h"
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace Mayhem
{
	// ---------- Caches de ressources partagées ----------

	static Ref<Shader> LoadShaderCached(const std::string& _path)
	{
		static std::unordered_map<std::string, Ref<Shader>> s_Cache;

		if (_path.empty())
			return nullptr;

		auto it = s_Cache.find(_path);
		if (it != s_Cache.end())
			return it->second;

		if (!std::filesystem::exists(_path))
		{
			std::cerr << "[Material] Shader introuvable : " << _path << std::endl;
			return nullptr;
		}

		Ref<Shader> shader = Shader::Create(_path);
		s_Cache[_path] = shader;
		return shader;
	}

	static Ref<Texture2D> LoadTextureCached(const std::string& _path)
	{
		static std::unordered_map<std::string, Ref<Texture2D>> s_Cache;

		if (_path.empty())
			return nullptr;

		auto it = s_Cache.find(_path);
		if (it != s_Cache.end())
			return it->second;

		if (!std::filesystem::exists(_path))
		{
			std::cerr << "[Material] Texture introuvable : " << _path << std::endl;
			return nullptr;
		}

		Ref<Texture2D> texture = Texture2D::Create(_path);
		s_Cache[_path] = texture;
		return texture;
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

	const Ref<Texture2D>& Material::GetAlbedoMap()
	{
		if (m_LoadedAlbedoMapPath != m_AlbedoMapPath)
		{
			m_AlbedoMap = LoadTextureCached(m_AlbedoMapPath);
			m_LoadedAlbedoMapPath = m_AlbedoMapPath;
		}
		return m_AlbedoMap;
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

	static std::string ReadString(const YAML::Node& _n, const std::string& _def)
	{
		return _n ? _n.as<std::string>() : _def;
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

	Ref<Material> Material::Load(const std::string& _path)
	{
		static std::unordered_map<std::string, Ref<Material>> s_Cache;

		auto it = s_Cache.find(_path);
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

		mat->m_ShaderPath        = ReadString(node["Shader"], mat->m_ShaderPath);
		mat->m_AlbedoColor       = ReadVec4(node["AlbedoColor"], mat->m_AlbedoColor);
		mat->m_AlbedoMapPath     = ReadString(node["AlbedoMap"], mat->m_AlbedoMapPath);
		mat->m_Tiling            = ReadVec2(node["Tiling"], mat->m_Tiling);
		mat->m_Offset            = ReadVec2(node["Offset"], mat->m_Offset);
		mat->m_AlphaCutoff       = ReadFloat(node["AlphaCutoff"], mat->m_AlphaCutoff);
		mat->m_Lit               = ReadBool(node["Lit"], mat->m_Lit);
		mat->m_SpecularStrength  = ReadFloat(node["SpecularStrength"], mat->m_SpecularStrength);
		mat->m_Shininess         = ReadFloat(node["Shininess"], mat->m_Shininess);
		mat->m_EmissionColor     = ReadVec3(node["EmissionColor"], mat->m_EmissionColor);
		mat->m_EmissionIntensity = ReadFloat(node["EmissionIntensity"], mat->m_EmissionIntensity);

		s_Cache[_path] = mat;
		return mat;
	}

	bool Material::Save(const std::string& _path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "Shader" << YAML::Value << m_ShaderPath;
		WriteVec4(out, "AlbedoColor", m_AlbedoColor);
		out << YAML::Key << "AlbedoMap" << YAML::Value << m_AlbedoMapPath;
		WriteVec2(out, "Tiling", m_Tiling);
		WriteVec2(out, "Offset", m_Offset);
		out << YAML::Key << "AlphaCutoff" << YAML::Value << m_AlphaCutoff;
		out << YAML::Key << "Lit" << YAML::Value << m_Lit;
		out << YAML::Key << "SpecularStrength" << YAML::Value << m_SpecularStrength;
		out << YAML::Key << "Shininess" << YAML::Value << m_Shininess;
		WriteVec3(out, "EmissionColor", m_EmissionColor);
		out << YAML::Key << "EmissionIntensity" << YAML::Value << m_EmissionIntensity;

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
			return mat;
		}();
		return s_DefaultUnlit;
	}
}
