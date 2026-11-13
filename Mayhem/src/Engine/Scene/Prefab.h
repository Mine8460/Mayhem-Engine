#pragma once

#include "Scene.h"
#include "Entity.h"

#include <filesystem>
#include <string>
#include <cstdint>

namespace Mayhem
{
	// Prefab = une entité et tous ses enfants enregistrés dans un fichier ".prefab" (même format YAML que les scènes).
	// Une instance dans la scène porte un PrefabComponent (sur sa racine) qui pointe vers le fichier par identifiant d'asset.
	class Prefab
	{
	public:
		static constexpr const char* Extension = ".prefab";

		// Enregistre _root et ses descendants dans _path (écrase le fichier)
		static bool Save(Entity _root, const std::string& _path);

		// Crée "<dossier>/<nom de l'entité>.prefab" (nom unique) et fait de _entity une instance de ce prefab.
		// Renvoie le chemin du fichier créé ("" en cas d'échec).
		static std::string CreateFromEntity(Entity _entity, const std::filesystem::path& _folder);

		// Ajoute une copie du prefab dans la scène (nouveaux UUID), sous _parent (nul = racine).
		// _rootUUID != 0 : UUID imposé pour la racine (utilisé par Revert pour garder la même entité).
		// Renvoie la racine créée (entité nulle si le fichier est illisible).
		static Entity Instantiate(Scene& _scene, const std::string& _path, Entity _parent = {}, uint64_t _rootUUID = 0);

		// Racine de l'instance de prefab qui contient _entity (elle-même ou un parent). Entité nulle sinon.
		static Entity FindInstanceRoot(Entity _entity);

		// Chemin actuel du prefab d'une instance ("" si introuvable)
		static std::string GetPrefabPath(Entity _instanceRoot);

		// Remplace l'instance par une copie neuve du prefab (garde nom, transform de la racine, parent et UUID).
		// Renvoie la nouvelle racine (l'ancienne Entity n'est plus valide), ou _instanceRoot si le prefab est illisible.
		static Entity Revert(Scene& _scene, Entity _instanceRoot);

		// Enregistre l'instance dans son prefab, puis met à jour les autres instances de la scène.
		// Renvoie le nombre d'autres instances mises à jour (-1 en cas d'échec).
		static int Apply(Scene& _scene, Entity _instanceRoot);

		// Coupe le lien avec le prefab (l'entité redevient une entité normale)
		static void Unpack(Entity _instanceRoot);

		// Infos pour l'inspecteur du Content Browser
		static bool ReadSummary(const std::string& _path, std::string& _outRootName, size_t& _outEntityCount);
	};
}
