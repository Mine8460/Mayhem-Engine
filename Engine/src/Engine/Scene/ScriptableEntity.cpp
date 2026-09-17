#include "Enpch.h"
#include "ScriptableEntity.h"
#include "Components.h"

namespace Engine
{
	void ScriptableEntity::Bind(Entity _entity)
	{
		m_Entity = _entity;
	}
}

