#include "Enpch.h"
#include "Components.h"
#include "Entity.h"

namespace Mayhem
{

	Entity::Entity(entt::entity _id, Scene* _scene)
		: m_EntityHandle(_id), m_Scene(_scene)
	{

	}

	Entity::~Entity()
	{

	}

	// ---------- Hiérarchie ----------

	Entity Entity::GetParent()
	{
		return m_Scene->GetEntityByUUID(GetComponent<RelationshipComponent>().m_Parent);
	}

	std::vector<Entity> Entity::GetChildren()
	{
		std::vector<Entity> children;
		for (uint64_t id : GetComponent<RelationshipComponent>().m_Children)
		{
			Entity child = m_Scene->GetEntityByUUID(id);
			if (child)
				children.push_back(child);
		}
		return children;
	}

	bool Entity::SetParent(Entity _parent, bool _keepWorldTransform)
	{
		return m_Scene->SetParent(*this, _parent, _keepWorldTransform);
	}

	bool Entity::IsDescendantOf(Entity _ancestor)
	{
		return m_Scene->IsDescendantOf(*this, _ancestor);
	}

	glm::mat4 Entity::GetWorldTransform()
	{
		return m_Scene->GetWorldTransform(*this);
	}

}
