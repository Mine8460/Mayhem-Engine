#pragma once

#include "Camera.h"
#include <Engine/Core/Timestep.h>
#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/MouseEvent.h>

namespace Engine
{

	class OrthographicCamera : public Camera2
	{
	public:
		OrthographicCamera(float _left, float _right, float _bottom, float _top);
		
		void SetProjection(float _left, float _right, float _bottom, float _top);
	protected:
		void RecalculateViewMatrix(void) override;
	};

	class OrthographicCameraController : public CameraController
	{
	public:
		OrthographicCameraController(float _aspectration, bool _rotation = false);
	
		void OnUpdate(Timestep _ts);
		void OnEvent(Event& _e);

		void OnResize(float _w, float _h);

		OrthographicCamera& GetCamera(void) { return m_Camera; }
	private:
		bool OnMouseScrolled(MouseScrolledEvent& _e);
		bool OnWidowResized(WindowResizeEvent& _e);
	private:
		OrthographicCamera m_Camera;
	
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;

		bool m_Rotation;

		glm::vec3 m_CameraPosition = glm::vec3(0.0f);
		float m_CameraSpeed = 1.0f;

		float m_CameraRotation = 0.f;
		float m_CameraRotationSpeed = 90.f;
	};
}
