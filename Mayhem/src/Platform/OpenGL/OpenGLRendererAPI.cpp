#include "Enpch.h"
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

namespace Mayhem
{
	void OpenGLRendererAPI::Init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h)
	{
		glViewport(_x, _y, _w, _h);
	}

	void OpenGLRendererAPI::Clear(const glm::vec4& _color)
	{
		glClearColor(_color.r, _color.g, _color.b, _color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount)
	{
		if (_iCount == 0)
		{
			glDrawElements(GL_TRIANGLES, _VA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
		}
		else
		{
			glDrawElements(GL_TRIANGLES, _iCount, GL_UNSIGNED_INT, nullptr);
		}
	}

}
