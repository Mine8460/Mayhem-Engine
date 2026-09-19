#include <Enpch.h>
#include "Application.h"

#include "Logger.h"

#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/RenderCommand.h>

#include <GLFW/glfw3.h>

namespace Mayhem
{
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(std::string _name) : m_Name(_name)
	{
		s_Instance = this;

		m_Window = Scope<Window>(Window::Create(_name));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application(void)
	{
		Renderer::Shutdown();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = glfwGetTime(); // Platform::GetTime()
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minized)
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);
			}

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	void Application::Close(void)
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& _e)
	{
		EventDispatcher dispatcher(_e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClosed));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(_e);
			if (_e.Handled)
			{
				break;
			}
		}
	}

	void Application::PushLayer(Layer* _layer)
	{
		m_LayerStack.PushLayer(_layer);
	}

	void Application::PushOverlay(Layer* _overlay)
	{
		m_LayerStack.PushOverlay(_overlay);
	}

	bool Application::OnWindowClosed(WindowCloseEvent& _e)
	{
		m_Running = false;

		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& _e)
	{
		if (_e.GetWidth() == 0 || _e.GetHeight() == 0)
		{
			m_Minized = true;
			return false;
		}
		m_Minized = false;
		Renderer::OnWindowResize(_e.GetWidth(), _e.GetHeight());

		return false;
	}

}