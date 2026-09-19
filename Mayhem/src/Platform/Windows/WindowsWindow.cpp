#include <Enpch.h>
#include "WindowsWindow.h"

#include <Engine/Events/ApplicationEvent.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Events/MouseEvent.h>

#include <glad/glad.h>

namespace Mayhem
{
	static bool s_GLFWInitialized = false;

	static void GLFWerrorCallback(int _errror, const char* _description)
	{
		MAYHEM_CORE_ERROR(_description);
	}

	Window* Window::Create(std::string _name, const WindowProps& _props)
	{
		return new WindowsWindow(_name, _props);
	}

	WindowsWindow::WindowsWindow(std::string _name, const WindowProps& _props)
	{
		Init(_name, _props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool _enable)
	{
		if (_enable)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = _enable;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	void WindowsWindow::Init(std::string _name, const WindowProps& _props)
	{
		m_Data.title = _name;
		m_Data.width = _props.width;
		m_Data.height = _props.height;


		if (!s_GLFWInitialized)
		{
			int succes = glfwInit();

			glfwSetErrorCallback(GLFWerrorCallback);

			s_GLFWInitialized = true;
		}


		m_Window = glfwCreateWindow((int)_props.width, (int)_props.height, m_Data.title.c_str(), nullptr, nullptr);

		m_Context = new OpenGLContext(m_Window);
		m_Context->Init();

		

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW Event Callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* _window, int _w, int _h)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);
				data.width = _w;
				data.height = _h;

				WindowResizeEvent event(_w, _h);
				data.eventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* _window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);
				WindowCloseEvent event;
				data.eventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* _window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.eventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.eventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.eventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* _window, unsigned int character)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);
				KeyTypedEvent event(character);
				data.eventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* _window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.eventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.eventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* _window, double _x, double _y)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);

				MouseScrolledEvent event((float)_x, (float)_y);
				data.eventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* _window, double x, double y)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(_window);

				MouseMovedEvent event((float)x, (float)y);
				data.eventCallback(event);
			});
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}

}