#include "Enpch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include <Engine/Renderer/SceneRenderer.h>
#include <Engine/Renderer/RenderCommand.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <algorithm>

namespace Mayhem
{
	// Matrice -> position / rotation (euler, radians) / échelle.
	// Sans cisaillement : suffisant pour des transforms construites par TransformComponent.
	static void DecomposeInto(const glm::mat4& _m, TransformComponent& _tc)
	{
		glm::vec3 c0 = glm::vec3(_m[0]);
		glm::vec3 c1 = glm::vec3(_m[1]);
		glm::vec3 c2 = glm::vec3(_m[2]);

		glm::vec3 scale(glm::length(c0), glm::length(c1), glm::length(c2));
		if (glm::dot(glm::cross(c0, c1), c2) < 0.0f) // repère inversé = échelle négative (miroir)
			scale.x = -scale.x;

		const float eps = 1e-6f;
		glm::mat3 rotation(
			c0 / (std::abs(scale.x) > eps ? scale.x : eps),
			c1 / (std::abs(scale.y) > eps ? scale.y : eps),
			c2 / (std::abs(scale.z) > eps ? scale.z : eps));

		_tc.m_Translation = glm::vec3(_m[3]);
		_tc.m_Rotation = glm::eulerAngles(glm::quat_cast(rotation));
		_tc.m_Scale = scale;
	}

	// ---------- Copie de components ----------
	// Ajoute ici tout nouveau component pour qu'il soit copié en mode Play et dupliqué avec Ctrl+D.
	#define MAYHEM_COPYABLE_COMPONENTS TransformComponent, CameraComponent, SpriteRenderer, MeshRenderer, \
		DirectionalLight, PointLight, SpotLight, Skybox, NativeScriptComponent, PrefabComponent

	template<typename... T>
	static void CopyComponents(entt::registry& _dstRegistry, entt::entity _dst, entt::registry& _srcRegistry, entt::entity _src)
	{
		([&]()
		{
			if (auto* component = _srcRegistry.try_get<T>(_src))
			{
				// Copie d'abord : si source et destination sont le même registre,
				// emplace peut réallouer le stockage et invalider `component`
				T copy = *component;
				_dstRegistry.emplace_or_replace<T>(_dst, std::move(copy));
			}
		}(), ...);
	}

	// Une instance de script appartient à une seule entité : la copie repart de zéro
	static void ResetScriptInstance(entt::registry& _registry, entt::entity _entity)
	{
		if (auto* nsc = _registry.try_get<NativeScriptComponent>(_entity))
			nsc->m_Instance = nullptr;
	}

	// Garde position + rotation, retire l'échelle (une caméra ne doit jamais être étirée,
	// même si elle ou l'un de ses parents a une échelle non uniforme)
	static glm::mat4 RemoveScale(const glm::mat4& _m)
	{
		glm::mat4 result = _m;
		for (int i = 0; i < 3; i++)
		{
			glm::vec3 axis = glm::vec3(_m[i]);
			float length = glm::length(axis);
			result[i] = glm::vec4(length > 1e-6f ? axis / length : axis, 0.0f);
		}
		return result;
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		OnRuntimeStop();
	}

	Ref<Scene> Scene::Copy(const Ref<Scene>& _other)
	{
		Ref<Scene> newScene = MakeRef<Scene>();
		newScene->m_ViewportWidth = _other->m_ViewportWidth;
		newScene->m_ViewportHeight = _other->m_ViewportHeight;

		entt::registry& srcRegistry = _other->m_Registry;

		std::vector<entt::entity> sources;
		for (auto [entity] : srcRegistry.storage<entt::entity>().each())
			sources.push_back(entity);

		// Créées dans l'ordre inverse du parcours : la hiérarchie s'affiche dans le même ordre
		for (auto it = sources.rbegin(); it != sources.rend(); ++it)
		{
			entt::entity src = *it;
			uint64_t uuid = (uint64_t)srcRegistry.get<IDComponent>(src).m_ID;
			const std::string& name = srcRegistry.get<TagComponent>(src).Tag;

			Entity dst = newScene->CreateEntityWithUUID(name, uuid);

			// Mêmes UUID : les liens parent/enfant restent valides tels quels
			CopyComponents<RelationshipComponent, MAYHEM_COPYABLE_COMPONENTS>(newScene->m_Registry, dst, srcRegistry, src);
			ResetScriptInstance(newScene->m_Registry, dst);
		}

		// Les caméras prennent la taille du viewport
		newScene->OnViewportSize(newScene->m_ViewportWidth, newScene->m_ViewportHeight);
		return newScene;
	}

	Entity Scene::DuplicateEntity(Entity _entity)
	{
		if (!_entity)
			return Entity{ entt::null, this };

		return DuplicateEntityRecursive(_entity, _entity.GetParent());
	}

	Entity Scene::DuplicateEntityRecursive(Entity _source, Entity _newParent)
	{
		// Copie de la liste : créer des entités ne doit pas modifier ce qu'on parcourt
		std::vector<Entity> children = _source.GetChildren();

		Entity copy = CreateEntity(_source.GetComponent<TagComponent>().Tag);
		CopyComponents<MAYHEM_COPYABLE_COMPONENTS>(m_Registry, copy, m_Registry, _source);
		ResetScriptInstance(m_Registry, copy);

		// Transform locale identique sous le même parent = même position à l'écran
		SetParent(copy, _newParent, false);

		for (Entity child : children)
			DuplicateEntityRecursive(child, copy);

		return copy;
	}

	void Scene::OnRuntimeStop()
	{
		m_Registry.view<NativeScriptComponent>().each([](auto entity, NativeScriptComponent& nsc)
			{
				if (nsc.m_Instance && nsc.DestroyScript)
					nsc.DestroyScript(&nsc);
				nsc.m_Instance = nullptr;
			});
	}

	Entity Scene::CreateEntity(const std::string& _name)
	{
		return CreateEntityWithUUID(_name, (uint64_t)UUID());
	}

	Entity Scene::CreateEntityWithUUID(const std::string& _name, uint64_t _uuid)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(_uuid);
		entity.AddComponent<TagComponent>(_name);
		entity.AddComponent<TransformComponent>(glm::vec3(0.0f));
		entity.AddComponent<RelationshipComponent>();

		m_EntityMap[_uuid] = entity;
		return entity;
	}

	void Scene::DestroyEntity(Entity _entity)
	{
		if (!_entity || !m_Registry.valid(_entity))
			return;

		if (auto* rc = m_Registry.try_get<RelationshipComponent>(_entity))
		{
			// Copie : chaque enfant détruit se retire lui-même de la liste
			std::vector<uint64_t> children = rc->m_Children;
			for (uint64_t childId : children)
				DestroyEntity(GetEntityByUUID(childId));

			// Se retire de la liste de son parent
			SetParent(_entity, Entity{ entt::null, this }, false);
		}

		m_EntityMap.erase((uint64_t)_entity.GetUUID());
		m_Registry.destroy(_entity);
	}

	Entity Scene::GetEntityByUUID(uint64_t _uuid)
	{
		if (_uuid == 0)
			return Entity{ entt::null, this };

		auto it = m_EntityMap.find(_uuid);
		if (it == m_EntityMap.end() || !m_Registry.valid(it->second))
			return Entity{ entt::null, this };

		return Entity{ it->second, this };
	}

	// ---------- Hiérarchie ----------

	bool Scene::IsDescendantOf(Entity _entity, Entity _ancestor)
	{
		if (!_entity || !_ancestor)
			return false;

		Entity current = _entity.GetParent();
		while (current)
		{
			if (current == _ancestor)
				return true;
			current = current.GetParent();
		}
		return false;
	}

	bool Scene::SetParent(Entity _child, Entity _parent, bool _keepWorldTransform)
	{
		if (!_child)
			return false;

		// Interdit : se mettre soi-même en parent, ou l'un de ses descendants (boucle infinie)
		if (_parent && (_parent == _child || IsDescendantOf(_parent, _child)))
			return false;

		const glm::mat4 worldBefore = GetWorldTransform(_child);
		const uint64_t childId = (uint64_t)_child.GetUUID();
		auto& rc = _child.GetComponent<RelationshipComponent>();

		// Détache de l'ancien parent
		if (Entity oldParent = GetEntityByUUID(rc.m_Parent))
		{
			auto& siblings = oldParent.GetComponent<RelationshipComponent>().m_Children;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), childId), siblings.end());
		}
		rc.m_Parent = 0;

		// Attache au nouveau parent
		if (_parent)
		{
			rc.m_Parent = (uint64_t)_parent.GetUUID();
			_parent.GetComponent<RelationshipComponent>().m_Children.push_back(childId);
		}

		// Recalcule la transform locale pour que l'objet ne bouge pas à l'écran
		if (_keepWorldTransform)
		{
			glm::mat4 local = _parent ? glm::inverse(GetWorldTransform(_parent)) * worldBefore : worldBefore;
			DecomposeInto(local, _child.GetComponent<TransformComponent>());
		}

		return true;
	}

	glm::mat4 Scene::GetWorldTransform(Entity _entity)
	{
		glm::mat4 local = _entity.GetComponent<TransformComponent>().GetTransform();

		if (auto* rc = m_Registry.try_get<RelationshipComponent>(_entity))
		{
			if (Entity parent = GetEntityByUUID(rc->m_Parent))
				return GetWorldTransform(parent) * local;
		}
		return local;
	}

	// ---------- Mise à jour / rendu ----------

	void Scene::OnUpdateRuntime(Timestep _ts)
	{
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, NativeScriptComponent& nsc)
				{
					if (!nsc.m_Instance)
					{
						if (!nsc.InstantiateScript) // script pas encore lié (Bind<T>() non appelé)
							return;
						nsc.m_Instance = nsc.InstantiateScript();
						nsc.m_Instance->Bind({ entity, this });
						nsc.m_Instance->OnCreate();
					}
					nsc.m_Instance->OnUpdate(_ts);
				});

		}


		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			auto viewEach = view.each();
			for (auto [entity, transform, camera] : viewEach)
			{
				if (camera.m_Primary)
				{
					mainCamera = &camera.m_Camera;
					cameraTransform = RemoveScale(GetWorldTransform(Entity{ entity, this }));
					break;
				}
			}
		}

		// Render
		if (mainCamera != nullptr)
		{
			RenderCommand::Clear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
			SceneRenderer::BeginScene(*mainCamera, cameraTransform);

			auto skies = m_Registry.view<Skybox>();
			auto skiesEach = skies.each();
			for (auto [entity, sb] : skiesEach)
			{
				SceneRenderer::SkySettings sky;
				sky.Enabled = true;
				sky.ZenithColor = sb.m_ZenithColor;
				sky.HorizonColor = sb.m_HorizonColor;
				sky.GroundColor = sb.m_GroundColor;
				sky.SunSize = sb.m_SunSize;
				sky.SunGlow = sb.m_SunGlow;
				sky.Exposure = sb.m_Exposure;
				sky.EnvironmentIntensity = sb.m_EnvironmentIntensity;
				SceneRenderer::SetSky(sky);
				SceneRenderer::ColorSettings color;
				color.Exposure = sb.m_SceneExposure;
				color.ToneMapping = sb.m_ToneMapping;
				SceneRenderer::SetColorSettings(color);
				break; // un seul ciel
			}

			auto lights = m_Registry.view<TransformComponent, DirectionalLight>();
			auto lightsEach = lights.each();
			for (auto [entity, transform, light] : lightsEach)
			{
				SceneRenderer::SetDirectionalLight(GetWorldTransform(Entity{ entity, this }), light.m_Color, light.m_Intensity, light.m_Ambient);
				SceneRenderer::ShadowSettings shadows;
				shadows.Enabled = light.m_CastShadows;
				shadows.Distance = light.m_ShadowDistance;
				shadows.Bias = light.m_ShadowBias;
				shadows.ShowCascades = light.m_ShowCascades;
				SceneRenderer::SetShadowSettings(shadows);
				break; // une seule lumière directionnelle pour l'instant
			}
			SubmitLocalLights();

			auto spriteView = m_Registry.view<TransformComponent, SpriteRenderer>();
			auto spriteEach = spriteView.each();
			for (auto [entity, transform, sprite] : spriteEach)
			{
				SceneRenderer::DrawSprite(GetWorldTransform(Entity{ entity, this }), sprite, (int)entity);
			}

			auto meshView = m_Registry.view<TransformComponent, MeshRenderer>();
			auto meshEach = meshView.each();
			for (auto [entity, transform, mesh] : meshEach)
			{
				SceneRenderer::DrawMeshRenderer(GetWorldTransform(Entity{ entity, this }), mesh, (int)entity);
			}

			SceneRenderer::EndScene();
		}
		else
		{
			MAYHEM_CORE_WARN("No main camera");
		}
	}

	// Envoie les point lights et spot lights de la scène au renderer
	void Scene::SubmitLocalLights()
	{
		for (auto [entity, light] : m_Registry.view<PointLight>().each())
		{
			const glm::mat4 world = GetWorldTransform(Entity{ entity, this });
			SceneRenderer::AddPointLight(glm::vec3(world[3]), light.m_Color, light.m_Intensity, light.m_Range,
				light.m_CastShadows, light.m_ShadowBias);
		}

		for (auto [entity, light] : m_Registry.view<SpotLight>().each())
		{
			const glm::mat4 world = GetWorldTransform(Entity{ entity, this });
			const glm::vec3 direction = glm::vec3(world * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
			SceneRenderer::AddSpotLight(glm::vec3(world[3]), direction, light.m_Color, light.m_Intensity, light.m_Range,
				light.m_InnerAngle, light.m_OuterAngle, light.m_CastShadows, light.m_ShadowBias);
		}
	}

	void Scene::OnUpdateEditor(Timestep _ts, EditorCamera& _cam)
	{
		SceneRenderer::BeginScene(_cam);

		auto lights = m_Registry.view<TransformComponent, DirectionalLight>();
		auto lightsEach = lights.each();
		for (auto [entity, transform, light] : lightsEach)
		{
			SceneRenderer::SetDirectionalLight(GetWorldTransform(Entity{ entity, this }), light.m_Color, light.m_Intensity, light.m_Ambient);
			SceneRenderer::ShadowSettings shadows;
			shadows.Enabled = light.m_CastShadows;
			shadows.Distance = light.m_ShadowDistance;
			shadows.Bias = light.m_ShadowBias;
			shadows.ShowCascades = light.m_ShowCascades;
			SceneRenderer::SetShadowSettings(shadows);
			break; // une seule lumière directionnelle pour l'instant
		}
		SubmitLocalLights();

		auto skies = m_Registry.view<Skybox>();
		auto skiesEach = skies.each();
		for (auto [entity, sb] : skiesEach)
		{
			SceneRenderer::SkySettings sky;
			sky.Enabled = true;
			sky.ZenithColor = sb.m_ZenithColor;
			sky.HorizonColor = sb.m_HorizonColor;
			sky.GroundColor = sb.m_GroundColor;
			sky.SunSize = sb.m_SunSize;
			sky.SunGlow = sb.m_SunGlow;
			sky.Exposure = sb.m_Exposure;
				sky.EnvironmentIntensity = sb.m_EnvironmentIntensity;
			SceneRenderer::SetSky(sky);
			SceneRenderer::ColorSettings color;
			color.Exposure = sb.m_SceneExposure;
			color.ToneMapping = sb.m_ToneMapping;
			SceneRenderer::SetColorSettings(color);
			break; // un seul ciel
		}

		auto spriteView = m_Registry.view<TransformComponent, SpriteRenderer>();
		auto spriteEach = spriteView.each();
		for (auto [entity, transform, sprite] : spriteEach)
		{
			SceneRenderer::DrawSprite(GetWorldTransform(Entity{ entity, this }), sprite, (int)entity);
		}

		auto meshView = m_Registry.view<TransformComponent, MeshRenderer>();
		auto meshEach = meshView.each();
		for (auto [entity, transform, mesh] : meshEach)
		{
			SceneRenderer::DrawMeshRenderer(GetWorldTransform(Entity{ entity, this }), mesh, (int)entity);
		}

		SceneRenderer::EndScene();
	}

	void Scene::OnViewportSize(uint32_t _w, uint32_t _h)
	{
		// Taille inconnue (viewport pas encore affiché, fenêtre réduite...) : aspect ratio = 0/0 = NaN
		if (_w == 0 || _h == 0)
			return;

		m_ViewportWidth = _w;
		m_ViewportHeight = _h;

		// Resize our non-FixedAspectRation camera(s)
		auto view = m_Registry.view<CameraComponent>();
		auto viewEach = view.each();

		for (auto [entity, camera] : viewEach)
		{
			if (!camera.m_FixedAspectRatio)
			{
				camera.m_Camera.SetViewportSize(_w, _h);
			}
		}
	}

	void Scene::RemoveAllEntities(void)
	{
		m_Registry.clear();
		m_EntityMap.clear();
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		auto viewEach = view.each();
		for (auto [entity, camera] : viewEach)
		{
			if (camera.m_Primary)
				return Entity{ entity, this };
		}
		return Entity{ entt::null, this };
	}

	void Scene::OnComponentAdded(Entity entity, Component* component)
	{
		CameraComponent* comp = dynamic_cast<CameraComponent*>(component);

		// Pas de taille connue (ex : scène en cours de chargement) : OnViewportSize s'en chargera plus tard
		if (comp && m_ViewportWidth > 0 && m_ViewportHeight > 0)
		{
			comp->m_Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}
}
