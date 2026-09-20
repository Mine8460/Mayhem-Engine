#include "Enpch.h"
#include "SceneSerializer.h"
#include <fstream>
#include "Components.h"

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};
}

namespace Mayhem
{
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}


	SceneSerializer::SceneSerializer(const Ref<Scene>& _scene)
		: m_Scene(_scene)
	{

	}

	void SceneSerializer::Serialize(const std::string& _path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << "Name";
		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;

		for (auto id : m_Scene->m_Registry.storage<entt::entity>())
		{
			Entity entity = { id, m_Scene.get() };
			if (!entity)
				return;
			SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(_path);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& _path)
	{
		// Not implemented
		MAYHEM_CORE_ASSERT(false, "");
	}

	bool SceneSerializer::Deserialize(const std::string& _path)
	{
		std::ifstream stream(_path);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		MAYHEM_CORE_INFO("Deserializing scene");

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tag = entity["TagComponent"];
				if (tag)
					name = tag["Tag"].as<std::string>();

				Entity deserializedEntity = m_Scene->CreateEntity(name);

				auto transform = entity["TransformComponent"];
				if (transform)
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.m_Translation = transform["Translation"].as<glm::vec3>();
					tc.m_Rotation = transform["Rotation"].as<glm::vec3>();
					tc.m_Scale = transform["Scale"].as<glm::vec3>();
				}

				auto cam = entity["CameraComponent"];
				if (cam)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto camProps = cam["Camera"];
					cc.m_Camera.SetProjectionType((SceneCamera::ProjectionType)camProps["ProjectionType"].as<int>());
					
					cc.m_Camera.SetFOV(camProps["PerspectiveFOV"].as<float>());
					cc.m_Camera.SetPerspectiveFarClip(camProps["PerspectiveNear"].as<float>());
					cc.m_Camera.SetPerspectiveFarClip(camProps["PerspectiveFar"].as<float>());
					
					cc.m_Camera.SetOrthographicSize(camProps["OrthographicSize"].as<float>());
					cc.m_Camera.SetOrthographicNearClip(camProps["OrthographicNear"].as<float>());
					cc.m_Camera.SetOrthographicFarClip(camProps["OrthographicFar"].as<float>());
				
					cc.m_Primary = cam["Primary"].as<bool>();
					cc.m_FixedAspectRatio = cam["FixedAspectRatio"].as<bool>();
				}

				auto sprite = entity["SpriteRenderer"];
				if (sprite)
				{
					auto& sc = deserializedEntity.AddComponent<SpriteRenderer>();
					sc.m_Color = sprite["Color"].as<glm::vec4>();
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& _path)
	{
		// Not implemented
		MAYHEM_CORE_ASSERT(false, "");
		return false;
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& _out, Entity _entity)
	{
		_out << YAML::BeginMap; // Entity
		_out << YAML::Key << "Entity";
		_out << YAML::Value << "12837192831273"; // TODO: Entity ID

		if (_entity.HasComponent<TagComponent>())
		{
			_out << YAML::Key << "TagComponent";
			_out << YAML::BeginMap;

			auto& tag = _entity.GetComponent<TagComponent>().Tag;
			_out << YAML::Key << "Tag" << YAML::Value << tag;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<TransformComponent>())
		{
			_out << YAML::Key << "TransformComponent";
			_out << YAML::BeginMap;

			auto& tc = _entity.GetComponent<TransformComponent>();
			_out << YAML::Key << "Translation" << YAML::Value << tc.m_Translation;
			_out << YAML::Key << "Rotation" << YAML::Value << tc.m_Rotation;
			_out << YAML::Key << "Scale" << YAML::Value << tc.m_Scale;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<CameraComponent>())
		{
			_out << YAML::Key << "CameraComponent";
			_out << YAML::BeginMap;

			auto& camComp = _entity.GetComponent<CameraComponent>();
			auto& cam = camComp.m_Camera;

			_out << YAML::Key << "Camera" << YAML::Value;
			_out << YAML::BeginMap; // Camera
			_out << YAML::Key << "PerspectiveFOV" << YAML::Value << (float)cam.GetFOV();
			_out << YAML::Key << "PerspectiveNear" << YAML::Value << (float)cam.GetPerspectiveNearClip();
			_out << YAML::Key << "PerspectiveFar" << YAML::Value << (float)cam.GetPerspectiveFarClip();
			_out << YAML::Key << "OrthographicSize" << YAML::Value << (float)cam.GetOrthographicSize();
			_out << YAML::Key << "OrthographicNear" << YAML::Value << (float)cam.GetOrthographicNearClip();
			_out << YAML::Key << "OrthographicFar" << YAML::Value << (float)cam.GetOrthographicFarClip();
			_out << YAML::Key << "ProjectionType" << YAML::Value << (int)cam.GetProjectionType();
			_out << YAML::EndMap;

			_out << YAML::Key << "Primary" << YAML::Value << camComp.m_Primary;
			_out << YAML::Key << "FixedAspectRatio" << YAML::Value << camComp.m_FixedAspectRatio;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<SpriteRenderer>())
		{
			_out << YAML::Key << "SpriteRenderer";
			_out << YAML::BeginMap;

			auto& sprite = _entity.GetComponent<SpriteRenderer>();
			_out << YAML::Key << "Color" << YAML::Value << sprite.m_Color;

			_out << YAML::EndMap;
		}

		_out << YAML::EndMap; // Entity
	}

}
