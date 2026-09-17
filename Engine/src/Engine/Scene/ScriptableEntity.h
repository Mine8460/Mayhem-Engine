#pragma once

#include "Entity.h"

namespace Engine
{
	class TransformComponent;

	class ScriptableEntity
	{
	public:
		ScriptableEntity(void) { }
		virtual ~ScriptableEntity() {}
		void Bind(Entity _entity);


		virtual void OnCreate() = 0;
		virtual void OnDestroy() = 0;
		virtual void OnUpdate(Timestep _ts) = 0;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(args);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		std::vector<Component*> GetComponents()
		{
			return m_Entity.GetComponents();
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Entity.HasComponent();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}
	protected:
	private:
		Entity m_Entity;
		friend class Scene;
	};
}
