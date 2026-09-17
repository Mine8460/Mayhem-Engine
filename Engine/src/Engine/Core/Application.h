#pragma once

#include "Core.h"
#include "Window.h"
#include <Engine/Events/ApplicationEvent.h>
#include "LayerStack.h"

#include <Engine/ImGui/ImGuiLayer.h>

#include <Engine/Core/Timestep.h>

namespace Engine
{
	class ENGINE_API Application
	{
	public:
		Application(void);
		virtual ~Application(void);

		void Run();

		void Close(void);

		void OnEvent(Event& _e);

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		void PushLayer(Layer* _layer);
		void PushOverlay(Layer* _overlay);

		inline Window& GetWindow() { return *m_Window; }
		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClosed(WindowCloseEvent& _e);
		bool OnWindowResize(WindowResizeEvent& _e);
	private:
		static Application* s_Instance;
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		bool m_Minized = false;

		float m_LastFrameTime = 0.0f;

		LayerStack m_LayerStack;
	};

	// To be defined in a CLIENT
	Application* CreateApplication();
}

