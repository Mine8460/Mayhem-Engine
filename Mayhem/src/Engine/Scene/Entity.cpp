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

}
