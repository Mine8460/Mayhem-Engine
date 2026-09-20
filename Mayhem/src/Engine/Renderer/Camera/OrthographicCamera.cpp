#include "Enpch.h"
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Core/Input.h>

namespace Mayhem
{
	OrthographicCamera::OrthographicCamera(float _left, float _right, float _bottom, float _top)
	{
		m_Position = glm::vec3(0.f);
		m_Rotation = glm::vec3(0.f);
		m_ProjectionMatrix = glm::ortho(_left, _right, _top, _bottom, -1.0f, 1.0f);
		m_ViewMatrix = glm::mat4(1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::SetProjection(float _left, float _right, float _bottom, float _top)
	{
		m_ProjectionMatrix = glm::ortho(_left, _right, _top, _bottom, -1.0f, 1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix(void)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), glm::vec3(0, 0, 1));

		m_ViewMatrix = glm::inverse(transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	OrthographicCameraController::OrthographicCameraController(float _aspectration, bool _rotation)
		: m_AspectRatio(_aspectration), m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio* m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel), m_Rotation(_rotation)
	{
		m_CameraPosition = glm::vec3(1.0f);
		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel);
	}

	void OrthographicCameraController::OnUpdate(Timestep _ts)
	{
		float dt = _ts.GetSeconds();

		if (Input::IsKeyPressed(Key::A))
			m_CameraPosition.x -= m_CameraSpeed * dt * m_ZoomLevel;

		if (Input::IsKeyPressed(Key::D))
			m_CameraPosition.x += m_CameraSpeed * dt * m_ZoomLevel;

		if (Input::IsKeyPressed(Key::S))
			m_CameraPosition.y -= m_CameraSpeed * dt * m_ZoomLevel;

		if (Input::IsKeyPressed(Key::W))
			m_CameraPosition.y += m_CameraSpeed * dt * m_ZoomLevel;

		if (m_Rotation)
		{
			if (Input::IsKeyPressed(Key::Q))
				m_CameraRotation += m_CameraRotationSpeed * dt;
			if (Input::IsKeyPressed(Key::E))
				m_CameraRotation -= m_CameraRotationSpeed * dt;

			m_Camera.SetRotation(glm::vec3(m_CameraRotation));
		}

		m_Camera.SetPosition(m_CameraPosition);
	}

	void OrthographicCameraController::OnEvent(Event& _e)
	{
		EventDispatcher dispatcher(_e);

		dispatcher.Dispatch<MouseScrolledEvent>(ENGINE_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(ENGINE_BIND_EVENT_FN(OrthographicCameraController::OnWidowResized));
	}

	void OrthographicCameraController::OnResize(float _w, float _h)
	{
		m_AspectRatio = _w / _h;
		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel);
	}

	bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& _e)
	{
		m_ZoomLevel -= _e.GetYOffset() * 0.2f;

		if (m_ZoomLevel < 0.1f)
		{
			m_ZoomLevel = 0.1f;
		}
		else if (m_ZoomLevel > 10.0f)
		{
			m_ZoomLevel = 10.0f;
		}

		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel);

		return false;
	}

	bool OrthographicCameraController::OnWidowResized(WindowResizeEvent& _e)
	{
		m_AspectRatio = (float)_e.GetWidth() / (float)_e.GetHeight();

		m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, m_ZoomLevel, -m_ZoomLevel);

		return false;
	}
}


