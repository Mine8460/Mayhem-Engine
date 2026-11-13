#pragma once

#include <Engine/Core/Core.h>
#include "Scene.h"
#include "Entity.h"
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

namespace Mayhem
{
	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& _scene);

		void Serialize(const std::string& _path);
		std::string SerializeToString();
		void SerializeRuntime(const std::string& _path);

		bool Deserialize(const std::string& _path);
		bool DeserializeRuntime(const std::string& _path);

		static void SerializeEntity(YAML::Emitter& _out, Entity _entity);
		static Entity DeserializeEntity(const YAML::Node& _node, Scene* _scene, uint64_t _uuid);
	private:
		Ref<Scene> m_Scene;
	};
}