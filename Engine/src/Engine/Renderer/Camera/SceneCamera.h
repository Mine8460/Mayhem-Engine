#pragma once

#include <Engine/Renderer/Camera/Camera.h>

namespace Engine
{
	class SceneCamera : public Camera
	{
	public:
		enum class ProjectionType
		{
			Perspective = 0,
			Orthographic = 1,
		};
	public:
		SceneCamera();
		SceneCamera(const glm::mat4& _projection);
		virtual ~SceneCamera() = default;

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType _type) { m_ProjectionType = _type; RecalculateProjection(); }

		void SetOrthographic(float _size, float _nearClip, float _farClip);
		void SetViewportSize(uint32_t _w, uint32_t _h);

		float GetOrthographicSize() const { return m_OrthoSize; }
		void SetOrthographicSize(float _s) { m_OrthoSize = _s; RecalculateProjection(); }

		float GetOrthographicNearClip() const { return m_OrthoNear; }
		float GetOrthographicFarClip() const { return m_OrthoFar; }
		void SetOrthographicNearClip(float _near) { m_OrthoNear = _near;RecalculateProjection(); }
		void SetOrthographicFarClip(float _far) { m_OrthoFar = _far;RecalculateProjection(); }

		float GetPerspectiveNearClip() const { return m_PersNear; }
		float GetPerspectiveFarClip() const { return m_PersFar; }
		void SetPerspectiveNearClip(float _near) { m_PersNear = _near;RecalculateProjection(); }
		void SetPerspectiveFarClip(float _far) { m_PersFar = _far;RecalculateProjection(); }

		float GetFOV() { return m_FOV; }
		void SetFOV(float _fov) { m_FOV = _fov;RecalculateProjection(); }
	private:
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		float m_OrthoSize = 10.0f;
		float m_OrthoNear = -1.0f, m_OrthoFar = 1.0f;

		float m_FOV = glm::radians(45.f);
		float m_PersNear = 0.01f, m_PersFar = 1000.0f;

		float m_AspectRatio = 0.0;
	};
}
