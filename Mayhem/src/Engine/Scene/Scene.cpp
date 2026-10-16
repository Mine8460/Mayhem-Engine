#include "Enpch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include <Engine/Renderer/SceneRenderer.h>
#include <Engine/Renderer/RenderCommand.h>
#include <Engine/Scene/ScriptableEntity.h>

namespace Mayhem
{

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const std::string& _name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>();
		entity.AddComponent<TagComponent>(_name);
		entity.AddComponent<TransformComponent>(glm::vec3(1.0f));
		return entity;
	}

	Entity Scene::CreateEntityWithUUID(const std::string& _name, uint64_t _uuid)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(_uuid);
		entity.AddComponent<TagComponent>(_name);
		entity.AddComponent<TransformComponent>(glm::vec3(0.0f));
		return entity;
	}

	void Scene::DestroyEntity(Entity _entity)
	{
		m_Registry.destroy(_entity);
	}

	void Scene::OnUpdateRuntime(Timestep _ts)
	{
		{
			m_Registry.view<NativeScriptComponent>().each([=](auto entity, NativeScriptComponent& nsc)
				{
					if (!nsc.m_Instance)
					{
						nsc.m_Instance = nsc.InstantiateScript();
						nsc.m_Instance->Bind({ entity, this });
						nsc.m_Instance->OnCreate();
					}
					nsc.m_Instance->OnUpdate(_ts);
				});

		}

		// Render sprites
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			{
				auto view = m_Registry.view<TransformComponent, CameraComponent>();
				auto viewEach = view.each();
				for (auto [entity, transform, camera] : viewEach)
				{
					if (camera.m_Primary)
					{
						mainCamera = &camera.m_Camera;
						cameraTransform = transform.GetTransform();
						break;
					}
				}
			}
		}

		auto lights = m_Registry.view<TransformComponent, DirectionalLight>();
		auto viewEach = lights.each();
		for (auto [entity, transform, light] : viewEach)
		{
			SceneRenderer::SetDirectionalLight(transform.GetTransform(), light.m_Color, light.m_Intensity, light.m_Ambient);
			break; // une seule lumière directionnelle pour l'instant
		}

		// Render 2D
		if (mainCamera != nullptr)
		{
			RenderCommand::Clear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
			SceneRenderer::BeginScene(*mainCamera, cameraTransform);

			auto spriteGroup = m_Registry.group<TransformComponent>(entt::get<SpriteRenderer>);

			for (auto entity : spriteGroup)
			{
				auto [transform, sprite] = spriteGroup.get<TransformComponent, SpriteRenderer>(entity);
				SceneRenderer::DrawQuad(transform.GetTransform(), sprite.m_Texture, sprite.m_Color, sprite.m_Tiling, (uint32_t)entity);
			}

			auto meshGroup = m_Registry.group<TransformComponent>(entt::get<MeshRenderer>);

			for (auto entity : meshGroup)
			{
				auto [transform, mesh] = meshGroup.get<TransformComponent, MeshRenderer>(entity);
				SceneRenderer::DrawCube(transform.GetTransform(), mesh.m_Texture, mesh.m_Color, mesh.m_Tiling, (uint32_t)entity);
			}

			SceneRenderer::EndScene();
		}
		else
		{
			MAYHEM_CORE_WARN("No main camera");
		}
	}

	void Scene::OnUpdateEditor(Timestep _ts, EditorCamera& _cam)
	{
		SceneRenderer::BeginScene(_cam);

		auto lights = m_Registry.view<TransformComponent, DirectionalLight>();
		auto viewEach = lights.each();
		for (auto [entity, transform, light] : viewEach)
		{
			SceneRenderer::SetDirectionalLight(transform.GetTransform(), light.m_Color, light.m_Intensity, light.m_Ambient);
			break; // une seule lumière directionnelle pour l'instant
		}

		//auto spriteGroup = m_Registry.group<TransformComponent>(entt::get<SpriteRenderer>);
		auto meshGroup = m_Registry.group<TransformComponent>(entt::get<MeshRenderer>);

		for (auto entity : meshGroup)
		{
			auto [transform, mesh] = meshGroup.get<TransformComponent, MeshRenderer>(entity);
			SceneRenderer::DrawCube(transform.GetTransform(), mesh.m_Texture, mesh.m_Color, mesh.m_Tiling, (uint32_t)entity);
		}

		//for (auto entity : spriteGroup)
		//{
		//	auto [transform, sprite] = spriteGroup.get<TransformComponent, SpriteRenderer>(entity);
		//	SceneRenderer::DrawQuad(transform.GetTransform(), sprite.m_Texture, sprite.m_Color, sprite.m_Tiling, (uint32_t)entity);
		//}

		SceneRenderer::EndScene();
	}

	void Scene::OnViewportSize(uint32_t _w, uint32_t _h)
	{
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

		if (comp)
		{
			comp->m_Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		}
	}
}
