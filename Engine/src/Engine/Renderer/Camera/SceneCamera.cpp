#include "Enpch.h"
#include "SceneCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	SceneCamera::SceneCamera() : Camera()
	{
		RecalculateProjection();
	}
	SceneCamera::SceneCamera(const glm::mat4& _projection) : Camera(_projection)
	{
		RecalculateProjection();
	}
	void SceneCamera::SetOrthographic(float _size, float _nearClip, float _farClip)
	{
		m_OrthoSize = _size;
		m_OrthoNear = _nearClip;
		m_OrthoFar = _farClip;

		RecalculateProjection();
	}

	void SceneCamera::SetViewportSize(uint32_t _w, uint32_t _h)
	{
		m_AspectRatio = (float)_w / (float)_h;

		RecalculateProjection();
	}

	void SceneCamera::RecalculateProjection()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_Projection = glm::perspective(m_FOV, m_AspectRatio, m_PersNear, m_PersFar);
		}
		else
		{
			float aspectRatio = m_AspectRatio;
			float orthoLeft = -0.5f * aspectRatio * m_OrthoSize;
			float orthoRight = 0.5f * aspectRatio * m_OrthoSize;
			float orthoBottom = -0.5f * m_OrthoSize;
			float orthoTop = 0.5f * m_OrthoSize;

			m_Projection = glm::ortho(orthoLeft, orthoRight,
				orthoBottom, orthoTop,
				m_OrthoNear, m_OrthoFar);
		}
	}

}
