#pragma once

#include <Engine/Renderer/Camera/SceneCamera.h>
#include <Engine/Core/Timestep.h>
#include <Engine/Renderer/SubTexture2D.h>
#include <Engine/Core/UUID.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
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
		float m_Tiling = 1.0f;
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

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

	};
}
