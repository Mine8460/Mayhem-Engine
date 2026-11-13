#include "Enpch.h"
#include "EditorCamera.h"

#include <Engine/Core/Input.h>
#include <Engine/Core/KeyCode.h>
#include <Engine/Core/MouseCode.h>

#include <glfw/glfw3.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace Mayhem
{
	static constexpr float s_RotationSpeed = 0.003f;   // radians par pixel
	static constexpr float s_FlyBoost = 3.0f;          // multiplicateur avec Shift
	static constexpr float s_MinFlySpeed = 0.1f;
	static constexpr float s_MaxFlySpeed = 500.0f;
	static constexpr float s_FocusDuration = 0.25f;    // secondes

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
	{
		UpdateView();
	}

	void EditorCamera::UpdateProjection()
	{
		if (m_ViewportWidth <= 0.0f || m_ViewportHeight <= 0.0f)
			return; // évite un aspect ratio de 0/0

		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdateView()
	{
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	// ---------- Choix du mode ----------

	void EditorCamera::UpdateControlMode()
	{
		const bool alt = Input::IsKeyPressed(Key::LeftAlt);
		const bool left = Input::IsMouseButtonPressed(Mouse::ButtonLeft);
		const bool middle = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);
		const bool right = Input::IsMouseButtonPressed(Mouse::ButtonRight);

		// Fin du mode en cours quand son bouton est relâché
		switch (m_Mode)
		{
		case ControlMode::Fly:   if (!right)  m_Mode = ControlMode::None; break;
		case ControlMode::Orbit: if (!left)   m_Mode = ControlMode::None; break;
		case ControlMode::Zoom:  if (!right)  m_Mode = ControlMode::None; break;
		case ControlMode::Pan:   if (!middle) m_Mode = ControlMode::None; break;
		default: break;
		}

		// Début d'un nouveau mode : seulement si la souris est sur le viewport
		if (m_Mode == ControlMode::None && m_ViewportHovered)
		{
			if (alt && left)
				m_Mode = ControlMode::Orbit;
			else if (alt && right)
				m_Mode = ControlMode::Zoom;
			else if (middle)
				m_Mode = ControlMode::Pan;
			else if (right)
				m_Mode = ControlMode::Fly;

			if (m_Mode != ControlMode::None)
				m_Focusing = false; // l'utilisateur reprend la main : on annule l'animation de Focus
		}
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		const float dt = ts.GetSeconds();

		// Delta souris en pixels, mis à jour chaque frame (pas de saut au début d'un mouvement)
		const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta = mouse - m_LastMousePosition;
		m_LastMousePosition = mouse;

		UpdateControlMode();

		switch (m_Mode)
		{
		case ControlMode::Fly:
			FlyLook(delta);
			FlyMove(dt);
			break;
		case ControlMode::Orbit:
			MouseRotate(delta);
			break;
		case ControlMode::Zoom:
			MouseZoom((delta.x + delta.y) * 0.003f);
			break;
		case ControlMode::Pan:
			MousePan(delta);
			break;
		default:
			break;
		}

		// Animation du Focus
		if (m_Focusing)
		{
			m_FocusProgress = std::min(m_FocusProgress + dt / s_FocusDuration, 1.0f);
			float t = m_FocusProgress * m_FocusProgress * (3.0f - 2.0f * m_FocusProgress); // smoothstep

			m_FocalPoint = glm::mix(m_FocusFromPoint, m_FocusToPoint, t);
			m_Distance = glm::mix(m_FocusFromDistance, m_FocusToDistance, t);

			if (m_FocusProgress >= 1.0f)
				m_Focusing = false;
		}

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(ENGINE_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		// Molette pendant le vol : change la vitesse de vol (comme Unity)
		if (m_Mode == ControlMode::Fly)
		{
			m_FlySpeed *= std::pow(1.2f, e.GetYOffset());
			m_FlySpeed = std::clamp(m_FlySpeed, s_MinFlySpeed, s_MaxFlySpeed);
			return false;
		}

		if (!m_ViewportHovered)
			return false;

		m_Focusing = false;
		MouseZoom(e.GetYOffset() * 0.1f);
		UpdateView();
		return false;
	}

	// ---------- Déplacements ----------

	// Regarder autour en restant sur place : la caméra tourne sur elle-même
	void EditorCamera::FlyLook(const glm::vec2& _delta)
	{
		const glm::vec3 position = CalculatePosition();

		m_Yaw += _delta.x * s_RotationSpeed;
		m_Pitch += _delta.y * s_RotationSpeed;
		ClampPitch();

		// La position reste fixe, c'est le point visé qui bouge
		m_FocalPoint = position + GetForwardDirection() * m_Distance;
	}

	void EditorCamera::FlyMove(float _dt)
	{
		glm::vec3 direction(0.0f);

		// ZQSD (AZERTY) et WASD (QWERTY) fonctionnent tous les deux
		if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Z)) direction += GetForwardDirection();
		if (Input::IsKeyPressed(Key::S))                                 direction -= GetForwardDirection();
		if (Input::IsKeyPressed(Key::D))                                 direction += GetRightDirection();
		if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Q))  direction -= GetRightDirection();
		if (Input::IsKeyPressed(Key::E))                                 direction += glm::vec3(0.0f, 1.0f, 0.0f);
		if (Input::IsKeyPressed(Key::LeftControl))                       direction -= glm::vec3(0.0f, 1.0f, 0.0f);

		if (glm::length(direction) < 0.0001f)
			return;

		float speed = m_FlySpeed;
		if (Input::IsKeyPressed(Key::LeftShift))
			speed *= s_FlyBoost;

		m_FocalPoint += glm::normalize(direction) * speed * _dt;
	}

	// Le point "attrapé" suit exactement le curseur
	void EditorCamera::MousePan(const glm::vec2& _delta)
	{
		const float worldPerPixel = 2.0f * m_Distance * std::tan(glm::radians(m_FOV) * 0.5f) / std::max(m_ViewportHeight, 1.0f);

		m_FocalPoint += -GetRightDirection() * _delta.x * worldPerPixel;
		m_FocalPoint += GetUpDirection() * _delta.y * worldPerPixel;
	}

	// Orbite autour du point visé
	void EditorCamera::MouseRotate(const glm::vec2& _delta)
	{
		m_Yaw += _delta.x * s_RotationSpeed;
		m_Pitch += _delta.y * s_RotationSpeed;
		ClampPitch();
	}

	void EditorCamera::MouseZoom(float _delta)
	{
		m_Distance -= _delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			// On "traverse" le point visé au lieu de rester bloqué dessus
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	void EditorCamera::ClampPitch()
	{
		// Évite de passer "par-dessus" (la vue se retournerait)
		const float limit = glm::radians(89.0f);
		m_Pitch = std::clamp(m_Pitch, -limit, limit);
	}

	void EditorCamera::Focus(const glm::vec3& _target, float _distance)
	{
		m_FocusFromPoint = m_FocalPoint;
		m_FocusToPoint = _target;
		m_FocusFromDistance = m_Distance;
		m_FocusToDistance = std::max(_distance, 1.0f);
		m_FocusProgress = 0.0f;
		m_Focusing = true;
	}

	// ---------- Orientation ----------

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

}
