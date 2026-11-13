#include "Enpch.h"
#include "SceneSerializer.h"
#include <fstream>
#include "Components.h"
#include <Engine/Core/AssetsManager.h>

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
	// ---------- Références d'assets ----------
	// Écrit le chemin (lisible) ET l'identifiant (survit aux renommages / déplacements)
	static void WriteAssetReference(YAML::Emitter& _out, const std::string& _key, const std::string& _path)
	{
		const std::string resolved = AssetsManager::Resolve(_path);
		_out << YAML::Key << _key << YAML::Value << (resolved.empty() ? std::string("None") : resolved);
		_out << YAML::Key << (_key + "ID") << YAML::Value << AssetsManager::GetID(resolved);
	}

	// Identifiant en priorité, chemin en secours (scènes enregistrées avant les identifiants). "" = aucun asset.
	static std::string ReadAssetReference(const YAML::Node& _node, const std::string& _key)
	{
		const std::string idKey = _key + "ID";
		if (_node[idKey])
		{
			std::string path = AssetsManager::GetPath(_node[idKey].as<uint64_t>());
			if (!path.empty())
				return path;
		}

		if (_node[_key])
		{
			std::string path = _node[_key].as<std::string>();
			if (path != "None")
				return AssetsManager::Resolve(path);
		}
		return "";
	}

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
		std::ofstream fout(_path);
		fout << SerializeToString();
	}

	std::string SceneSerializer::SerializeToString()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << "Name";
		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;

		// .each() et pas la boucle directe sur le storage : celle-ci passe AUSSI sur les entités détruites
		// (entt les garde dans le storage pour recycler leurs identifiants) -> GetComponent sur une entité morte = crash
		for (auto [id] : m_Scene->m_Registry.storage<entt::entity>().each())
		{
			Entity entity = { id, m_Scene.get() };
			SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		return std::string(out.c_str());
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

		// (enfant, parent) : les liens sont reconstruits après la création de toutes les entités,
		// car un enfant peut apparaître dans le fichier avant son parent
		std::vector<std::pair<uint64_t, uint64_t>> parentLinks;

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				if (entity["Parent"])
					parentLinks.emplace_back(uuid, entity["Parent"].as<uint64_t>());

				DeserializeEntity(entity, m_Scene.get(), uuid);
			}
		}

		// Transforms déjà locales dans le fichier : on ne les recalcule pas
		for (const auto& [childId, parentId] : parentLinks)
			m_Scene->SetParent(m_Scene->GetEntityByUUID(childId), m_Scene->GetEntityByUUID(parentId), false);

		return true;
	}

	Entity SceneSerializer::DeserializeEntity(const YAML::Node& _node, Scene* _scene, uint64_t _uuid)
	{
		std::string name;
		auto tag = _node["TagComponent"];
		if (tag)
			name = tag["Tag"].as<std::string>();

		Entity deserializedEntity = _scene->CreateEntityWithUUID(name, _uuid);

		auto transform = _node["TransformComponent"];
		if (transform)
		{
			auto& tc = deserializedEntity.GetComponent<TransformComponent>();
			tc.m_Translation = transform["Translation"].as<glm::vec3>();
			tc.m_Rotation = transform["Rotation"].as<glm::vec3>();
			tc.m_Scale = transform["Scale"].as<glm::vec3>();
		}

		auto cam = _node["CameraComponent"];
		if (cam)
		{
			auto& cc = deserializedEntity.AddComponent<CameraComponent>();

			auto camProps = cam["Camera"];
			cc.m_Camera.SetProjectionType((SceneCamera::ProjectionType)camProps["ProjectionType"].as<int>());

			cc.m_Camera.SetFOV(camProps["PerspectiveFOV"].as<float>());
			cc.m_Camera.SetPerspectiveNearClip(camProps["PerspectiveNear"].as<float>());
			cc.m_Camera.SetPerspectiveFarClip(camProps["PerspectiveFar"].as<float>());

			cc.m_Camera.SetOrthographicSize(camProps["OrthographicSize"].as<float>());
			cc.m_Camera.SetOrthographicNearClip(camProps["OrthographicNear"].as<float>());
			cc.m_Camera.SetOrthographicFarClip(camProps["OrthographicFar"].as<float>());

			cc.m_Primary = cam["Primary"].as<bool>();
			cc.m_FixedAspectRatio = cam["FixedAspectRatio"].as<bool>();
		}

		auto sprite = _node["SpriteRenderer"];
		if (sprite)
		{
			auto& sc = deserializedEntity.AddComponent<SpriteRenderer>();
			sc.m_Color = sprite["Color"].as<glm::vec4>();

			std::string texture = ReadAssetReference(sprite, "Texture");
			if (!texture.empty())
				sc.m_Texture = Texture2D::Create(texture);

			sc.m_MaterialPath = ReadAssetReference(sprite, "Material");
			if (!sc.m_MaterialPath.empty())
				sc.m_Material = Material::Load(sc.m_MaterialPath);
		}

		auto mesh = _node["MeshRenderer"];
		if (mesh)
		{
			auto& sc = deserializedEntity.AddComponent<MeshRenderer>();
			sc.m_Color = mesh["Color"].as<glm::vec4>();

			// Slots de matériaux (anciennes scènes : un seul "Material")
			if (auto materials = mesh["Materials"]; materials && materials.IsSequence())
			{
				for (const auto& slotNode : materials)
				{
					MeshRenderer::MaterialSlot slot;
					slot.Path = ReadAssetReference(slotNode, "Material");
					if (!slot.Path.empty())
						slot.Asset = Material::Load(slot.Path);
					sc.m_Materials.push_back(slot);
				}
			}
			else
			{
				MeshRenderer::MaterialSlot slot;
				slot.Path = ReadAssetReference(mesh, "Material");
				if (!slot.Path.empty())
					slot.Asset = Material::Load(slot.Path);
				sc.m_Materials.push_back(slot);
			}

			std::string meshPath = ReadAssetReference(mesh, "Mesh");
			if (!meshPath.empty())
				sc.SetMesh(meshPath); // garde le chemin même si le fichier est introuvable (pas de crash)
		}

		auto light = _node["DirectionalLight"];
		if (light)
		{
			auto& sc = deserializedEntity.AddComponent<DirectionalLight>();
			if (light["Color"])
				sc.m_Color = light["Color"].as<glm::vec3>();
			if (light["ShowCascades"])
				sc.m_ShowCascades = light["ShowCascades"].as<bool>();
			sc.m_Intensity = light["Intensity"].as<float>();
			sc.m_Ambient = light["Ambiant"].as<float>();
			sc.m_CastShadows = light["Shadows"].as<bool>();
			sc.m_ShadowDistance = light["ShadowDistance"].as<float>();
			sc.m_ShadowBias = light["ShadowBias"].as<float>();
		}

		auto pointLight = _node["PointLight"];
		if (pointLight)
		{
			auto& pl = deserializedEntity.AddComponent<PointLight>();
			pl.m_Color = pointLight["Color"].as<glm::vec3>();
			pl.m_Intensity = pointLight["Intensity"].as<float>();
			pl.m_Range = pointLight["Range"].as<float>();
			if (pointLight["Shadows"])
				pl.m_CastShadows = pointLight["Shadows"].as<bool>();
			if (pointLight["ShadowBias"])
				pl.m_ShadowBias = pointLight["ShadowBias"].as<float>();
		}

		auto spotLight = _node["SpotLight"];
		if (spotLight)
		{
			auto& sl = deserializedEntity.AddComponent<SpotLight>();
			sl.m_Color = spotLight["Color"].as<glm::vec3>();
			sl.m_Intensity = spotLight["Intensity"].as<float>();
			sl.m_Range = spotLight["Range"].as<float>();
			sl.m_OuterAngle = spotLight["OuterAngle"].as<float>();
			sl.m_InnerAngle = spotLight["InnerAngle"].as<float>();
			if (spotLight["Shadows"])
				sl.m_CastShadows = spotLight["Shadows"].as<bool>();
			if (spotLight["ShadowBias"])
				sl.m_ShadowBias = spotLight["ShadowBias"].as<float>();
		}

		auto skybox = _node["Skybox"];
		if (skybox)
		{
			auto& sky = deserializedEntity.AddComponent<Skybox>();
			sky.m_SunGlow = skybox["Glow"].as<float>();
			sky.m_SunSize = skybox["Size"].as<float>();
			sky.m_Exposure = skybox["Exposure"].as<float>();
			sky.m_GroundColor = skybox["Ground"].as<glm::vec3>();
			sky.m_HorizonColor = skybox["Horizon"].as<glm::vec3>();
			sky.m_ZenithColor = skybox["Zenith"].as<glm::vec3>();
			sky.m_SceneExposure = skybox["SceneExposure"].as<float>();
			sky.m_ToneMapping = skybox["ToneMapping"].as<bool>();
			if (skybox["EnvironmentIntensity"])
				sky.m_EnvironmentIntensity = skybox["EnvironmentIntensity"].as<float>();
		}

		auto prefab = _node["PrefabComponent"];
		if (prefab)
		{
			// Identifiant en priorité ; chemin en secours. Gardé même si le fichier est introuvable (lien non perdu).
			uint64_t prefabID = prefab["PrefabID"] ? prefab["PrefabID"].as<uint64_t>() : 0;
			if (prefabID == 0 && prefab["Prefab"] && prefab["Prefab"].as<std::string>() != "None")
				prefabID = AssetsManager::GetID(prefab["Prefab"].as<std::string>());
			if (prefabID != 0)
				deserializedEntity.AddComponent<PrefabComponent>(prefabID);
		}

		return deserializedEntity;
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
		_out << YAML::Value << (uint64_t)_entity.GetComponent<IDComponent>().m_ID; // TODO: Entity ID

		if (_entity.HasComponent<RelationshipComponent>())
		{
			uint64_t parent = _entity.GetComponent<RelationshipComponent>().m_Parent;
			if (parent != 0)
				_out << YAML::Key << "Parent" << YAML::Value << parent;
		}

		if (_entity.HasComponent<PrefabComponent>())
		{
			_out << YAML::Key << "PrefabComponent";
			_out << YAML::BeginMap;

			// Écrit à la main (pas WriteAssetReference) : si le fichier est introuvable, l'identifiant est gardé
			const uint64_t prefabID = _entity.GetComponent<PrefabComponent>().m_PrefabID;
			const std::string prefabPath = AssetsManager::GetPath(prefabID);
			_out << YAML::Key << "Prefab" << YAML::Value << (prefabPath.empty() ? std::string("None") : prefabPath);
			_out << YAML::Key << "PrefabID" << YAML::Value << prefabID;

			_out << YAML::EndMap;
		}

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
			WriteAssetReference(_out, "Texture", sprite.m_Texture ? sprite.m_Texture->GetPath() : "");
			WriteAssetReference(_out, "Material", sprite.m_Material ? sprite.m_Material->GetPath() : sprite.m_MaterialPath);

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<MeshRenderer>())
		{
			_out << YAML::Key << "MeshRenderer";
			_out << YAML::BeginMap;

			auto& mesh = _entity.GetComponent<MeshRenderer>();
			_out << YAML::Key << "Color" << YAML::Value << mesh.m_Color;
			// Si le fichier est introuvable, on garde quand même son chemin (rien n'est perdu à la sauvegarde)
			_out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
			for (const MeshRenderer::MaterialSlot& slot : mesh.m_Materials)
			{
				_out << YAML::BeginMap;
				WriteAssetReference(_out, "Material", slot.Asset ? slot.Asset->GetPath() : slot.Path);
				_out << YAML::EndMap;
			}
			_out << YAML::EndSeq;
			WriteAssetReference(_out, "Mesh", mesh.m_Mesh ? mesh.m_Mesh->GetPath() : mesh.m_MeshPath);

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<DirectionalLight>())
		{
			_out << YAML::Key << "DirectionalLight";
			_out << YAML::BeginMap;

			auto& light = _entity.GetComponent<DirectionalLight>();
			_out << YAML::Key << "Color" << YAML::Value << light.m_Color;
			_out << YAML::Key << "ShowCascades" << YAML::Value << light.m_ShowCascades;
			_out << YAML::Key << "Intensity" << YAML::Value << light.m_Intensity;
			_out << YAML::Key << "Ambiant" << YAML::Value << light.m_Ambient;
			_out << YAML::Key << "Shadows" << YAML::Value << light.m_CastShadows;
			_out << YAML::Key << "ShadowDistance" << YAML::Value << light.m_ShadowDistance;
			_out << YAML::Key << "ShadowBias" << YAML::Value << light.m_ShadowBias;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<PointLight>())
		{
			_out << YAML::Key << "PointLight";
			_out << YAML::BeginMap;

			auto& light = _entity.GetComponent<PointLight>();
			_out << YAML::Key << "Color" << YAML::Value << light.m_Color;
			_out << YAML::Key << "Intensity" << YAML::Value << light.m_Intensity;
			_out << YAML::Key << "Range" << YAML::Value << light.m_Range;
			_out << YAML::Key << "Shadows" << YAML::Value << light.m_CastShadows;
			_out << YAML::Key << "ShadowBias" << YAML::Value << light.m_ShadowBias;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<SpotLight>())
		{
			_out << YAML::Key << "SpotLight";
			_out << YAML::BeginMap;

			auto& light = _entity.GetComponent<SpotLight>();
			_out << YAML::Key << "Color" << YAML::Value << light.m_Color;
			_out << YAML::Key << "Intensity" << YAML::Value << light.m_Intensity;
			_out << YAML::Key << "Range" << YAML::Value << light.m_Range;
			_out << YAML::Key << "OuterAngle" << YAML::Value << light.m_OuterAngle;
			_out << YAML::Key << "InnerAngle" << YAML::Value << light.m_InnerAngle;
			_out << YAML::Key << "Shadows" << YAML::Value << light.m_CastShadows;
			_out << YAML::Key << "ShadowBias" << YAML::Value << light.m_ShadowBias;

			_out << YAML::EndMap;
		}

		if (_entity.HasComponent<Skybox>())
		{
			_out << YAML::Key << "Skybox";
			_out << YAML::BeginMap;

			auto& sky = _entity.GetComponent<Skybox>();
			_out << YAML::Key << "Glow" << YAML::Value << sky.m_SunGlow;
			_out << YAML::Key << "Exposure" << YAML::Value << sky.m_Exposure;
			_out << YAML::Key << "Size" << YAML::Value << sky.m_SunSize;
			_out << YAML::Key << "Ground" << YAML::Value << sky.m_GroundColor;
			_out << YAML::Key << "Horizon" << YAML::Value << sky.m_HorizonColor;
			_out << YAML::Key << "Zenith" << YAML::Value << sky.m_ZenithColor;
			_out << YAML::Key << "SceneExposure" << YAML::Value << sky.m_SceneExposure;
			_out << YAML::Key << "ToneMapping" << YAML::Value << sky.m_ToneMapping;
			_out << YAML::Key << "EnvironmentIntensity" << YAML::Value << sky.m_EnvironmentIntensity;

			_out << YAML::EndMap;
		}
		_out << YAML::EndMap; // Entity
	}

}
