#pragma once

#include <Engine/Core/Timestep.h>
#include <Engine/Events/MouseEvent.h>
#include "Camera.h"
#include <glm/glm.hpp>

namespace Mayhem
{
	// Caméra de l'éditeur, contrôles façon vue "Scene" d'Unity :
	//   Clic droit maintenu  : regarder autour + ZQSD/WASD pour voler, E = monter, Ctrl = descendre, Shift = x3
	//   Molette (clic droit) : vitesse de vol
	//   Alt + clic gauche    : orbiter autour du point visé
	//   Alt + clic droit     : zoom glissé
	//   Clic molette         : déplacer la vue (pan)
	//   Molette              : zoom
	//   Focus(...)           : cadrer un objet (touche F dans l'éditeur)
	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float ratio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		// Une manipulation ne commence que si la souris est sur le viewport
		// (elle continue ensuite tant que le bouton est maintenu, même en sortant du viewport)
		void SetViewportHovered(bool _hovered) { m_ViewportHovered = _hovered; }

		// Vrai pendant le mode vol (clic droit) : l'éditeur doit alors ignorer W/E/R/Q (raccourcis du gizmo)
		bool IsFlying() const { return m_Mode == ControlMode::Fly; }
		// Vrai pendant n'importe quelle manipulation de la caméra
		bool IsManipulating() const { return m_Mode != ControlMode::None; }

		// Centre la vue sur _target, à _distance (transition douce)
		void Focus(const glm::vec3& _target, float _distance);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline float GetFlySpeed() const { return m_FlySpeed; }
		inline void SetFlySpeed(float _speed) { m_FlySpeed = _speed; }

		inline void SetViewportSize(float w, float h) { m_ViewportWidth = w; m_ViewportHeight = h; UpdateProjection(); }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; };
		glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
	private:
		enum class ControlMode { None, Fly, Orbit, Pan, Zoom };

		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void UpdateControlMode();
		void FlyLook(const glm::vec2& _delta);
		void FlyMove(float _dt);
		void MousePan(const glm::vec2& _delta);
		void MouseRotate(const glm::vec2& _delta);
		void MouseZoom(float _delta);
		void ClampPitch();

		glm::vec3 CalculatePosition() const;

		float ZoomSpeed() const;
	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;

		// Contrôles
		ControlMode m_Mode = ControlMode::None;
		bool m_ViewportHovered = false;
		float m_FlySpeed = 5.0f; // mètres par seconde

		// Animation du Focus (touche F)
		bool m_Focusing = false;
		float m_FocusProgress = 0.0f;
		glm::vec3 m_FocusFromPoint = { 0.0f, 0.0f, 0.0f }, m_FocusToPoint = { 0.0f, 0.0f, 0.0f };
		float m_FocusFromDistance = 10.0f, m_FocusToDistance = 10.0f;
	};
}
