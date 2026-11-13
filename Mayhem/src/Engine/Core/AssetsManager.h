#pragma once
#include <Engine/Renderer/Texture.h>
//#include <Engine/Core/Core.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Mayhem
{
	using AssetID = uint64_t; // 0 = aucun asset

	// Gestion des assets :
	//  - cache des textures (GetTexture)
	//  - identifiants uniques (comme les GUID d'Unity) : chaque fichier du dossier assets a un
	//    "<fichier>.meta" qui contient son identifiant. Les scènes et matériaux référencent les assets
	//    par identifiant : renommer ou déplacer un fichier dans l'éditeur ne casse plus rien.
	class AssetsManager
	{
	public:
		static AssetsManager* GetInstance();

		AssetsManager();
		~AssetsManager();

		// Texture en cache (même instance pour un même fichier, même renommé / déplacé)
		Ref<Texture2D> GetTexture(std::string _path);

		// ---------- Identifiants d'assets ----------

		// Parcourt le dossier des assets et crée les .meta manquants. À appeler au démarrage.
		static void Init(const std::filesystem::path& _root = "assets");

		// Identifiant d'un fichier (crée son .meta s'il n'existe pas). 0 si ce n'est pas un asset.
		// Accepte aussi un ancien chemin (asset renommé / déplacé pendant la session).
		static AssetID GetID(const std::string& _path);

		// Chemin actuel d'un asset ("" si inconnu)
		static std::string GetPath(AssetID _id);

		// Chemin actuel d'un asset à partir d'un chemin éventuellement périmé
		static std::string Resolve(const std::string& _path);

		// À appeler APRÈS avoir déplacé / renommé un fichier ou un dossier (et son .meta)
		static void OnMoved(const std::filesystem::path& _from, const std::filesystem::path& _to);
		// À appeler APRÈS avoir supprimé un fichier ou un dossier
		static void OnDeleted(const std::filesystem::path& _path);

		static bool IsAssetFile(const std::filesystem::path& _path);
		static std::filesystem::path GetMetaPath(const std::filesystem::path& _path);
		static std::string Normalize(const std::filesystem::path& _path);

	private:
		AssetID RegisterFile(const std::filesystem::path& _path);
		void Register(const std::string& _path, AssetID _id);

	private:
		std::unordered_map<std::string, Ref<Texture2D>> m_Textures;
		static AssetsManager* m_Instance;

		// Identifiants
		std::filesystem::path m_Root = "assets";
		std::unordered_map<AssetID, std::string> m_IDToPath;
		std::unordered_map<std::string, AssetID> m_PathToID;
		std::unordered_map<std::string, AssetID> m_Aliases; // anciens chemins (renommés / déplacés pendant la session)
	};

}
