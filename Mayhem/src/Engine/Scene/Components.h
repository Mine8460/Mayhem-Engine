#pragma once

#include <Engine/Renderer/Camera/SceneCamera.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Renderer/SubTexture2D.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Core/UUID.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Mayhem
{
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;
	};

	class TagComponent : public Component
	{
	public:
		std::string Tag = "Unnamed";
		TagComponent() = default;
		TagComponent(const std::string& _tag) : Tag(_tag)
		{

		}

		operator std::string& () { return Tag; }
		operator const std::string& () const { return Tag; }
	};

	class IDComponent : public Component
	{
	public:
		IDComponent() : m_ID()
		{

		}
		IDComponent(uint64_t _uuid) : m_ID(_uuid)
		{

		}
		IDComponent(const IDComponent&) = default;

		UUID m_ID;
	};

	// Lien parent / enfants. Stocké en UUID (pas en entt::entity) pour survivre à la sauvegarde.
	// Ajouté automatiquement à chaque entité par Scene::CreateEntity.
	class RelationshipComponent : public Component
	{
	public:
		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;

		uint64_t m_Parent = 0;              // 0 = racine
		std::vector<uint64_t> m_Children;   // dans l'ordre d'affichage
	};

	// Posé sur la racine d'une instance de prefab : lien vers le fichier .prefab (par identifiant d'asset)
	class PrefabComponent : public Component
	{
	public:
		PrefabComponent() = default;
		PrefabComponent(const PrefabComponent&) = default;
		PrefabComponent(uint64_t _prefabID) : m_PrefabID(_prefabID) {}

		uint64_t m_PrefabID = 0;
	};

	class TransformComponent : public Component
	{
	public:
		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& _translation) : m_Translation(_translation)
		{

		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(m_Rotation));

			return glm::translate(glm::mat4(1.0f), m_Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), m_Scale);
		}

		glm::vec3 m_Translation = { 0.0f,0.0f,0.0f };
		glm::vec3 m_Rotation = { 0.0f,0.0f,0.0f };
		glm::vec3 m_Scale = { 1.0f,1.0f,1.0f };
	};

	class SpriteRenderer : public Component
	{
	public:
		SpriteRenderer() = default;
		SpriteRenderer(const SpriteRenderer&) = default;
		SpriteRenderer(glm::vec4 _color) : m_Color(_color)
		{

		}

		void ChangeTexture(Ref<Texture2D> _texture) { m_Texture = _texture; }
		void RemoveTexture(void) { m_Texture.reset(); }

		glm::vec4 m_Color = glm::vec4(1.0f);
		Ref<Texture2D> m_Texture;

		std::string m_MaterialPath;
		Ref<Material> m_Material; // nullptr = matériau par défaut
	};

	class MeshRenderer : public Component
	{
	public:
		MeshRenderer() = default;
		MeshRenderer(const MeshRenderer&) = default;
		MeshRenderer(glm::vec4 _color) : m_Color(_color)
		{

		}

		// Un slot de matériau (un par matériau du fichier 3D)
		struct MaterialSlot
		{
			std::string Path;
			Ref<Material> Asset; // nullptr = matériau par défaut
		};

		// (pas de glm::max ici : la macro max de Windows.h casse l'appel dans l'éditeur)
		static uint32_t AtLeastOne(uint32_t _count) { return _count > 0 ? _count : 1u; }

		// Charge le mesh et ajuste le nombre de slots de matériaux à celui du mesh
		// (les matériaux déjà assignés sont conservés)
		void SetMesh(const std::string& _path)
		{
			m_MeshPath = _path;
			m_Mesh = Mesh::Create(_path);
			if (m_Mesh)
				m_Materials.resize(AtLeastOne(m_Mesh->GetMaterialSlotCount()));
		}

		uint32_t GetMaterialSlotCount() const
		{
			return AtLeastOne(m_Mesh ? m_Mesh->GetMaterialSlotCount() : (uint32_t)m_Materials.size());
		}

		// Matériau du slot (chargé à la demande). nullptr = matériau par défaut.
		const Ref<Material>& GetMaterial(uint32_t _slot)
		{
			static const Ref<Material> s_None;
			if (_slot >= m_Materials.size())
				return s_None;

			MaterialSlot& slot = m_Materials[_slot];
			if (!slot.Asset && !slot.Path.empty())
				slot.Asset = Material::Load(slot.Path);
			return slot.Asset;
		}

		void SetMaterial(uint32_t _slot, const Ref<Material>& _material)
		{
			if (_slot >= m_Materials.size())
				m_Materials.resize(_slot + 1);
			m_Materials[_slot].Asset = _material;
			m_Materials[_slot].Path = _material ? _material->GetPath() : std::string();
		}

		glm::vec4 m_Color = glm::vec4(1.0f);
		std::vector<MaterialSlot> m_Materials; // index = MaterialIndex des sous-meshes

		std::string m_MeshPath;
		Ref<Mesh> m_Mesh;
	};

	class CameraComponent : public Component
	{
	public:
		CameraComponent() {}
		CameraComponent(const CameraComponent&) = default;

		SceneCamera m_Camera;
		bool m_Primary = true;
		bool m_FixedAspectRatio = false;
	};

	class DirectionalLight : public Component
	{
	public:
		DirectionalLight() {}
		DirectionalLight(const DirectionalLight&) = default;

		glm::vec3 m_Color = { 1.0f, 1.0f, 1.0f };
		float m_Intensity = 1.0f;
		float m_Ambient = 0.2f;

		bool m_CastShadows = true;
		float m_ShadowDistance = 100.f;
		float m_ShadowBias = 0.002f;
		bool m_ShowCascades = false;
	};

	// Lumière omnidirectionnelle (ampoule, torche…)
	class PointLight : public Component
	{
	public:
		PointLight() {}
		PointLight(const PointLight&) = default;

		glm::vec3 m_Color = { 1.0f, 1.0f, 1.0f };
		float m_Intensity = 5.0f; // 1 = comme un soleil d'intensité 1 vu à 1 m ; décroît avec le carré de la distance
		float m_Range = 10.0f;    // au-delà, la lumière est coupée (en mètres)

		bool m_CastShadows = true;
		float m_ShadowBias = 0.05f; // en mètres : monte si des rayures apparaissent sur les surfaces éclairées, baisse si l'ombre se décolle
	};

	// Lumière en cône (projecteur, lampe torche). Elle éclaire vers son axe -Z local.
	class SpotLight : public Component
	{
	public:
		SpotLight() {}
		SpotLight(const SpotLight&) = default;

		glm::vec3 m_Color = { 1.0f, 1.0f, 1.0f };
		float m_Intensity = 5.0f;
		float m_Range = 10.0f;
		float m_OuterAngle = 45.0f; // angle total du cône, en degrés
		float m_InnerAngle = 30.0f; // angle total de la zone pleine lumière (au-delà : fondu jusqu'au bord)

		bool m_CastShadows = true;
		float m_ShadowBias = 0.05f; // en mètres
	};

	class Skybox : public Component
	{
	public:
		Skybox() = default;
		Skybox(const Skybox&) = default;

		glm::vec3 m_ZenithColor = { 0.25f, 0.45f, 0.85f };
		glm::vec3 m_HorizonColor = { 0.70f, 0.80f, 0.95f };
		glm::vec3 m_GroundColor = { 0.35f, 0.32f, 0.30f };
		float m_SunSize = 1.5f;   // en degrés
		float m_SunGlow = 0.6f;
		float m_Exposure = 1.0f;
		float m_SceneExposure = 1.0f;
		bool m_ToneMapping = true;
		float m_EnvironmentIntensity = 1.0f; // éclairage ambiant + reflets (IBL) venant du ciel
	};

	class ScriptableEntity;

	class NativeScriptComponent : public Component
	{
	public:

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->m_Instance; nsc->m_Instance = nullptr; };
		}
	public:
		ScriptableEntity* m_Instance = nullptr;

		// nullptr tant que Bind<T>() n'a pas été appelé (sinon pointeurs non initialisés = crash)
		ScriptableEntity* (*InstantiateScript)() = nullptr;
		void (*DestroyScript)(NativeScriptComponent*) = nullptr;

	};
}
