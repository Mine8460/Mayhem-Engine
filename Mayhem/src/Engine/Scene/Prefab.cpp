#include "Enpch.h"
#include "Prefab.h"
#include "Components.h"
#include "SceneSerializer.h"
#include <Engine/Core/AssetsManager.h>
#include <Engine/Core/UUID.h>

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Mayhem
{
	// ---------- Outils ----------

	// La racine d'abord, puis chaque enfant et sa descendance (l'ordre des enfants est conservé au chargement)
	static void CollectDepthFirst(Entity _entity, std::vector<Entity>& _out)
	{
		_out.push_back(_entity);
		for (Entity child : _entity.GetChildren())
			CollectDepthFirst(child, _out);
	}

	static bool LoadPrefabFile(const std::string& _path, YAML::Node& _outData)
	{
		if (_path.empty() || !std::filesystem::is_regular_file(_path))
			return false;

		try
		{
			_outData = YAML::LoadFile(_path);
		}
		catch (const YAML::Exception& e)
		{
			return false;
		}

		return _outData["Prefab"] && _outData["Entities"] && _outData["Entities"].IsSequence() && _outData["Entities"].size() > 0;
	}

	// Caractères refusés par Windows dans un nom de fichier
	static std::string SanitizeFileName(std::string _name)
	{
		for (char& c : _name)
			if (std::string("\\/:*?\"<>|").find(c) != std::string::npos)
				c = '_';

		while (!_name.empty() && (_name.back() == ' ' || _name.back() == '.'))
			_name.pop_back();

		return _name.empty() ? "Prefab" : _name;
	}

	// ---------- Enregistrement ----------

	bool Prefab::Save(Entity _root, const std::string& _path)
	{
		if (!_root)
			return false;

		std::vector<Entity> entities;
		CollectDepthFirst(_root, entities);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::Value << _root.GetComponent<TagComponent>().Tag;
		out << YAML::Key << "Root" << YAML::Value << (uint64_t)_root.GetUUID();
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (Entity entity : entities)
			SceneSerializer::SerializeEntity(out, entity);
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream file(_path, std::ios::out | std::ios::trunc);
		if (!file)
			return false;
		file << out.c_str();
		file.close();

		AssetsManager::GetID(_path); // crée le .meta si c'est un nouveau fichier
		return true;
	}

	std::string Prefab::CreateFromEntity(Entity _entity, const std::filesystem::path& _folder)
	{
		if (!_entity)
			return "";

		// "Arbre.prefab", puis "Arbre 1.prefab"... si le nom est pris
		const std::string base = SanitizeFileName(_entity.GetComponent<TagComponent>().Tag);
		std::filesystem::path path = _folder / (base + Extension);
		for (int i = 1; std::filesystem::exists(path); i++)
			path = _folder / (base + " " + std::to_string(i) + Extension);

		const std::string key = AssetsManager::Normalize(path);

		// Comme Unity : l'entité d'origine devient une instance du nouveau prefab
		if (!Save(_entity, key))
			return "";

		const AssetID id = AssetsManager::GetID(key);
		if (_entity.HasComponent<PrefabComponent>())
			_entity.GetComponent<PrefabComponent>().m_PrefabID = id;
		else
			_entity.AddComponent<PrefabComponent>(id);

		// Le fichier contient maintenant aussi le lien (la racine de chaque instance pointera vers lui)
		Save(_entity, key);
		return key;
	}

	// ---------- Instanciation ----------

	static Entity InstantiateFromData(Scene& _scene, const YAML::Node& _data, AssetID _prefabID, Entity _parent, uint64_t _rootUUID)
	{
		const YAML::Node entities = _data["Entities"];
		const uint64_t fileRoot = _data["Root"] ? _data["Root"].as<uint64_t>() : entities[0]["Entity"].as<uint64_t>();

		// Ancien UUID (dans le fichier) -> nouvel UUID : plusieurs instances peuvent coexister dans une scène
		std::unordered_map<uint64_t, uint64_t> remap;
		for (const auto& node : entities)
		{
			const uint64_t oldID = node["Entity"].as<uint64_t>();
			remap[oldID] = (oldID == fileRoot && _rootUUID != 0) ? _rootUUID : (uint64_t)UUID();
		}

		Entity root{ entt::null, &_scene };
		std::vector<std::pair<Entity, uint64_t>> links; // (entité, nouvel UUID du parent)

		for (const auto& node : entities)
		{
			const uint64_t oldID = node["Entity"].as<uint64_t>();
			Entity entity = SceneSerializer::DeserializeEntity(node, &_scene, remap[oldID]);

			if (oldID == fileRoot)
			{
				root = entity;
				continue;
			}

			// Parent hors du prefab (fichier modifié à la main...) : rattaché à la racine
			uint64_t parent = node["Parent"] ? node["Parent"].as<uint64_t>() : 0;
			auto it = remap.find(parent);
			links.emplace_back(entity, it != remap.end() ? it->second : 0);
		}

		if (!root)
			return root;

		// Dans l'ordre du fichier (profondeur d'abord) : les enfants gardent leur ordre.
		// Transforms déjà locales : on ne les recalcule pas.
		for (auto& [entity, parentID] : links)
		{
			Entity parent = parentID != 0 ? _scene.GetEntityByUUID(parentID) : root;
			_scene.SetParent(entity, parent ? parent : root, false);
		}

		if (_parent)
			_scene.SetParent(root, _parent, false);

		if (root.HasComponent<PrefabComponent>())
			root.GetComponent<PrefabComponent>().m_PrefabID = _prefabID;
		else
			root.AddComponent<PrefabComponent>(_prefabID);

		return root;
	}

	Entity Prefab::Instantiate(Scene& _scene, const std::string& _path, Entity _parent, uint64_t _rootUUID)
	{
		const std::string path = AssetsManager::Resolve(_path);

		YAML::Node data;
		if (!LoadPrefabFile(path, data))
			return Entity{ entt::null, &_scene };

		return InstantiateFromData(_scene, data, AssetsManager::GetID(path), _parent, _rootUUID);
	}

	// ---------- Instances ----------

	Entity Prefab::FindInstanceRoot(Entity _entity)
	{
		Entity current = _entity;
		while (current)
		{
			if (current.HasComponent<PrefabComponent>())
				return current;
			current = current.GetParent();
		}
		return current; // nulle
	}

	std::string Prefab::GetPrefabPath(Entity _instanceRoot)
	{
		if (!_instanceRoot || !_instanceRoot.HasComponent<PrefabComponent>())
			return "";

		std::string path = AssetsManager::GetPath(_instanceRoot.GetComponent<PrefabComponent>().m_PrefabID);
		return std::filesystem::is_regular_file(path) ? path : "";
	}

	Entity Prefab::Revert(Scene& _scene, Entity _instanceRoot)
	{
		const std::string path = GetPrefabPath(_instanceRoot);

		// Fichier lu AVANT de détruire l'instance : rien n'est perdu s'il est illisible
		YAML::Node data;
		if (!LoadPrefabFile(path, data))
			return _instanceRoot;

		// Ce que l'instance garde : son nom, sa place dans la scène, son UUID
		const TransformComponent transform = _instanceRoot.GetComponent<TransformComponent>();
		const std::string name = _instanceRoot.GetComponent<TagComponent>().Tag;
		const uint64_t uuid = (uint64_t)_instanceRoot.GetUUID();
		const AssetID prefabID = _instanceRoot.GetComponent<PrefabComponent>().m_PrefabID;
		Entity parent = _instanceRoot.GetParent();

		int siblingIndex = -1;
		if (parent)
		{
			const auto& siblings = parent.GetComponent<RelationshipComponent>().m_Children;
			auto it = std::find(siblings.begin(), siblings.end(), uuid);
			if (it != siblings.end())
				siblingIndex = (int)(it - siblings.begin());
		}

		_scene.DestroyEntity(_instanceRoot);

		Entity root = InstantiateFromData(_scene, data, prefabID, parent, uuid);
		if (!root)
			return root;

		root.GetComponent<TransformComponent>() = transform;
		root.GetComponent<TagComponent>().Tag = name;

		// Même position dans la liste des enfants du parent
		if (parent && siblingIndex >= 0)
		{
			auto& siblings = parent.GetComponent<RelationshipComponent>().m_Children;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), uuid), siblings.end());
			siblings.insert(siblings.begin() + (std::min<size_t>)((size_t)siblingIndex, siblings.size()), uuid);
		}

		return root;
	}

	int Prefab::Apply(Scene& _scene, Entity _instanceRoot)
	{
		const std::string path = GetPrefabPath(_instanceRoot);
		if (path.empty() || !Save(_instanceRoot, path))
			return -1;

		const AssetID prefabID = _instanceRoot.GetComponent<PrefabComponent>().m_PrefabID;
		const uint64_t appliedUUID = (uint64_t)_instanceRoot.GetUUID();

		// UUID d'abord : Revert détruit et recrée des entités (on ne parcourt pas la vue pendant ce temps)
		std::vector<uint64_t> others;
		auto view = _scene.GetAllEntitiesWith<PrefabComponent, IDComponent>();
		auto each = view.each();
		for (auto [entity, prefab, id] : each)
		{
			if (prefab.m_PrefabID == prefabID && (uint64_t)id.m_ID != appliedUUID)
				others.push_back((uint64_t)id.m_ID);
		}

		int updated = 0;
		for (uint64_t uuid : others)
		{
			Entity instance = _scene.GetEntityByUUID(uuid);
			if (!instance)
				continue; // détruite en mettant à jour une instance précédente (instance imbriquée)

			// Instance contenue dans celle qu'on applique : déjà enregistrée telle quelle, on n'y touche pas
			if (_scene.IsDescendantOf(instance, _instanceRoot))
				continue;

			Revert(_scene, instance);
			updated++;
		}
		return updated;
	}

	void Prefab::Unpack(Entity _instanceRoot)
	{
		if (_instanceRoot && _instanceRoot.HasComponent<PrefabComponent>())
			_instanceRoot.RemoveComponent<PrefabComponent>();
	}

	bool Prefab::ReadSummary(const std::string& _path, std::string& _outRootName, size_t& _outEntityCount)
	{
		YAML::Node data;
		if (!LoadPrefabFile(AssetsManager::Resolve(_path), data))
			return false;

		_outRootName = data["Prefab"].as<std::string>();
		_outEntityCount = data["Entities"].size();
		return true;
	}
}
