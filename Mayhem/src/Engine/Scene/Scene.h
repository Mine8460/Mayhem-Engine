#pragma once

#include <entt.hpp>
#include <Engine/Core/Timestep.h>

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
		void DestroyEntity(Entity _entity);

		void OnUpdate(Timestep _ts);
		void OnViewportSize(uint32_t _w, uint32_t _h);
	private:
		void OnComponentAdded(Entity entity, Component* component);


		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;
	};
}
