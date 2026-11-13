#pragma once

#include <entt.hpp>
#include <Engine/Core/Timestep.h>
#include <Engine/Renderer/Camera/EditorCamera.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <cstdint>

namespace Mayhem
{
	class Entity;
	class Component;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& _name = "Entity");
		Entity CreateEntityWithUUID(const std::string& _name, uint64_t _uuid);
		void DestroyEntity(Entity _entity); // détruit aussi tous les enfants

		Entity GetEntityByUUID(uint64_t _uuid); // entité nulle si introuvable

		// Copie complète (mêmes UUID) : utilisée pour le mode Play, la scène d'origine reste intacte
		static Ref<Scene> Copy(const Ref<Scene>& _other);
		// Duplique l'entité et tous ses enfants (nouveaux UUID), sous le même parent
		Entity DuplicateEntity(Entity _entity);
		// Détruit les instances de scripts (à appeler en sortant du mode Play)
		void OnRuntimeStop();

		// ---------- Hiérarchie ----------
		// _parent nul = détacher (l'entité redevient racine).
		// Refuse (renvoie false) si _parent est l'entité elle-même ou l'un de ses descendants.
		bool SetParent(Entity _child, Entity _parent, bool _keepWorldTransform = true);
		bool IsDescendantOf(Entity _entity, Entity _ancestor);
		// Transform finale : parent(s) * locale
		glm::mat4 GetWorldTransform(Entity _entity);

		void OnUpdateRuntime(Timestep _ts);
		void OnUpdateEditor(Timestep _ts, EditorCamera& _cam);
		void OnViewportSize(uint32_t _w, uint32_t _h);

		void RemoveAllEntities(void);
		Entity GetPrimaryCameraEntity();

		// Toutes les entités qui ont ces components (pour l'éditeur, les outils...)
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
	private:
		void SubmitLocalLights(); // point lights + spot lights vers le renderer
		void OnComponentAdded(Entity entity, Component* component);
		Entity DuplicateEntityRecursive(Entity _source, Entity _newParent);


		entt::registry m_Registry;
		std::unordered_map<uint64_t, entt::entity> m_EntityMap; // UUID -> entité
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};
}
