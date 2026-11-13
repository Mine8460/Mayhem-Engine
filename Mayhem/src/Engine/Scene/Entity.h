#pragma once
//#include "Components.h"
#include <entt.hpp>
#include "Components.h"
#include "Scene.h"

namespace Mayhem
{
	class Component;

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity _id, Scene* _scene);
		Entity(const Entity& other) = default;
		~Entity();

		template<typename T>
		T& GetComponent()
		{
			if (m_Scene->m_Registry.valid(m_EntityHandle))
				return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		std::vector<Component*> GetComponents()
		{
			return m_Components;
		}

		UUID GetUUID()
		{
			return GetComponent<IDComponent>().m_ID;
		}

		// ---------- Hiérarchie ----------
		Entity GetParent();                       // entité nulle si racine
		std::vector<Entity> GetChildren();
		// _keepWorldTransform = true : l'objet ne bouge pas à l'écran (sa transform locale est recalculée)
		bool SetParent(Entity _parent, bool _keepWorldTransform = true);
		bool IsDescendantOf(Entity _ancestor);
		glm::mat4 GetWorldTransform();

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			MAYHEM_CORE_ASSERT(!HasComponent<T>(), "Entity already has this component!");

			T* component = &m_Scene->m_Registry.emplace<T>(
				m_EntityHandle,
				std::forward<Args>(args)...
			);
			m_Scene->OnComponentAdded(*this, component);
			m_Components.push_back(dynamic_cast<Component*>(component));

			return *component;
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			MAYHEM_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");

			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		bool IsValid() const
		{
			return m_Scene && m_EntityHandle != entt::null && m_Scene->m_Registry.valid(m_EntityHandle);
		}

		operator bool() const { return (uint32_t)m_EntityHandle != entt::null; }
		bool operator==(const Entity& _other)
		{
			return m_EntityHandle == _other.m_EntityHandle && m_Scene == _other.m_Scene;
		}
		bool operator!=(const Entity& _other)
		{
			return m_EntityHandle != _other.m_EntityHandle || m_Scene != _other.m_Scene;
		}
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }
		operator entt::entity() const { return m_EntityHandle; }
	private:

	private:
		std::vector<Component*> m_Components;

		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};
}
