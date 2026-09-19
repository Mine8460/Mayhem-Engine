#include "Enpch.h"
#include "OpenGLContext.h"

#include <glad/glad.h>

namespace Mayhem
{
	OpenGLContext::OpenGLContext(GLFWwindow* _winHandle) : m_WindowHandle(_winHandle)
	{

	}

	void Mayhem::OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		MAYHEM_CORE_INFO("OpenGL Info :");
		MAYHEM_CORE_INFO((const char*)glGetString(GL_VENDOR));
		MAYHEM_CORE_INFO((const char*)glGetString(GL_RENDERER));
		MAYHEM_CORE_INFO((const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

}
