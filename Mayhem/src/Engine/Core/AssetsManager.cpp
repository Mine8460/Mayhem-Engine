#include "Enpch.h"
#include "AssetsManager.h"

#include <Engine/Core/UUID.h>

#include <fstream>
#include <sstream>
#include <vector>

namespace Mayhem
{
	AssetsManager* AssetsManager::m_Instance = nullptr;

	// ---------- Outils ----------

	// _path est _parent lui-même ou se trouve dedans (chemins normalisés)
	static bool IsInsidePath(const std::string& _path, const std::string& _parent)
	{
		return _path == _parent || (_path.size() > _parent.size() && _path.compare(0, _parent.size(), _parent) == 0 && _path[_parent.size()] == '/');
	}

	// Lit "GUID: 1234" dans un .meta de fichier. 0 si absent ou illisible.
	static AssetID ReadMeta(const std::filesystem::path& _meta)
	{
		std::ifstream file(_meta);
		std::string line;
		while (std::getline(file, line))
		{
			if (line.rfind("GUID:", 0) == 0)
			{
				std::istringstream value(line.substr(5));
				AssetID id = 0;
				value >> id;
				return id;
			}
		}
		return 0;
	}

	static void WriteMeta(const std::filesystem::path& _meta, AssetID _id)
	{
		std::ofstream file(_meta, std::ios::out | std::ios::trunc);
		file << "GUID: " << _id << std::endl;
	}

	// ---------- Singleton ----------

	AssetsManager* AssetsManager::GetInstance()
	{
		if (m_Instance == nullptr)
			m_Instance = new AssetsManager();

		return m_Instance;
	}

	AssetsManager::AssetsManager()
	{
	}

	AssetsManager::~AssetsManager()
	{
		m_Textures.clear();
	}

	// ---------- Textures ----------

	Ref<Texture2D> AssetsManager::GetTexture(std::string _path)
	{
		// Chemin actuel + clé = identifiant : un même fichier (même renommé) = une seule texture
		const std::string path = Resolve(_path);
		const AssetID id = GetID(path);
		const std::string key = id != 0 ? "#" + std::to_string(id) : path;

		auto it = m_Textures.find(key);
		if (it != m_Textures.end())
			return it->second;

		Ref<Texture2D> texture = Texture2D::CreateFromAssetsManager(path);
		m_Textures[key] = texture;
		return texture;
	}

	// ---------- Identifiants ----------

	std::string AssetsManager::Normalize(const std::filesystem::path& _path)
	{
		return _path.lexically_normal().generic_string();
	}

	std::filesystem::path AssetsManager::GetMetaPath(const std::filesystem::path& _path)
	{
		return std::filesystem::path(_path.string() + ".meta");
	}

	bool AssetsManager::IsAssetFile(const std::filesystem::path& _path)
	{
		std::error_code error;
		return std::filesystem::is_regular_file(_path, error) && _path.extension() != ".meta";
	}

	void AssetsManager::Register(const std::string& _path, AssetID _id)
	{
		m_IDToPath[_id] = _path;
		m_PathToID[_path] = _id;
		m_Aliases.erase(_path);
	}

	// Lit (ou crée) le .meta d'un fichier et l'enregistre
	AssetID AssetsManager::RegisterFile(const std::filesystem::path& _path)
	{
		const std::string key = Normalize(_path);
		const std::filesystem::path meta = GetMetaPath(_path);

		AssetID id = ReadMeta(meta);

		// Identifiant déjà utilisé par un autre fichier existant (ex : fichier copié avec son .meta) : on en génère un neuf
		auto existing = m_IDToPath.find(id);
		const bool duplicate = id != 0 && existing != m_IDToPath.end() && existing->second != key && std::filesystem::exists(existing->second);

		if (id == 0 || duplicate)
		{
			id = (uint64_t)UUID();
			WriteMeta(meta, id);
		}

		Register(key, id);
		return id;
	}

	void AssetsManager::Init(const std::filesystem::path& _root)
	{
		AssetsManager& self = *GetInstance();
		self.m_Root = _root;
		self.m_IDToPath.clear();
		self.m_PathToID.clear();

		std::error_code error;
		for (auto& entry : std::filesystem::recursive_directory_iterator(_root, error))
		{
			if (IsAssetFile(entry.path()))
				self.RegisterFile(entry.path());
		}
	}

	AssetID AssetsManager::GetID(const std::string& _path)
	{
		if (_path.empty())
			return 0;

		AssetsManager& self = *GetInstance();
		const std::string key = Normalize(_path);

		auto it = self.m_PathToID.find(key);
		if (it != self.m_PathToID.end())
			return it->second;

		// Nouveau fichier (ajouté depuis l'explorateur, créé par l'éditeur...).
		// Testé avant les alias : un vrai fichier à ce chemin a priorité sur un ancien nom.
		if (IsAssetFile(key))
			return self.RegisterFile(key);

		// Ancien chemin d'un asset renommé / déplacé pendant la session
		auto alias = self.m_Aliases.find(key);
		if (alias != self.m_Aliases.end())
			return alias->second;

		return 0;
	}

	std::string AssetsManager::GetPath(AssetID _id)
	{
		if (_id == 0)
			return "";

		AssetsManager& self = *GetInstance();
		auto it = self.m_IDToPath.find(_id);
		if (it != self.m_IDToPath.end())
			return it->second;

		// Inconnu : un fichier a peut-être été ajouté / déplacé hors de l'éditeur, on reparcourt une fois
		Init(self.m_Root);
		it = self.m_IDToPath.find(_id);
		return it != self.m_IDToPath.end() ? it->second : "";
	}

	std::string AssetsManager::Resolve(const std::string& _path)
	{
		if (_path.empty())
			return "";

		AssetID id = GetID(_path);
		if (id != 0)
		{
			std::string current = GetPath(id);
			if (!current.empty())
				return current;
		}
		return Normalize(_path);
	}

	void AssetsManager::OnMoved(const std::filesystem::path& _from, const std::filesystem::path& _to)
	{
		AssetsManager& self = *GetInstance();
		const std::string from = Normalize(_from);
		const std::string to = Normalize(_to);

		// Tous les assets concernés : le fichier lui-même, ou tout le contenu d'un dossier
		std::vector<std::pair<std::string, AssetID>> moved;
		for (const auto& [path, id] : self.m_PathToID)
			if (IsInsidePath(path, from))
				moved.emplace_back(path, id);

		for (const auto& [oldPath, id] : moved)
		{
			std::string newPath = to + oldPath.substr(from.size());
			self.m_PathToID.erase(oldPath);
			self.m_Aliases[oldPath] = id; // les objets déjà chargés avec l'ancien chemin restent valides
			self.Register(newPath, id);
		}
	}

	void AssetsManager::OnDeleted(const std::filesystem::path& _path)
	{
		AssetsManager& self = *GetInstance();
		const std::string target = Normalize(_path);

		for (auto it = self.m_PathToID.begin(); it != self.m_PathToID.end(); )
		{
			if (IsInsidePath(it->first, target))
			{
				self.m_IDToPath.erase(it->second);
				it = self.m_PathToID.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = self.m_Aliases.begin(); it != self.m_Aliases.end(); )
			it = IsInsidePath(it->first, target) ? self.m_Aliases.erase(it) : std::next(it);
	}
}
