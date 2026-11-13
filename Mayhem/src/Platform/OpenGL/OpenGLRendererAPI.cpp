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

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount, uint32_t _firstIndex)
	{
		_VA->Bind();

		uint32_t count = _iCount ? _iCount : _VA->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (const void*)((uintptr_t)_firstIndex * sizeof(uint32_t)));
	}

	void OpenGLRendererAPI::SetCullMode(CullMode _mode)
	{
		if (_mode == CullMode::None)
		{
			glDisable(GL_CULL_FACE);
			return;
		}

		glEnable(GL_CULL_FACE);
		glCullFace(_mode == CullMode::Back ? GL_BACK : GL_FRONT);
	}

	void OpenGLRendererAPI::SetWireframe(bool _enabled)
	{
		if (_enabled)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// Les arêtes sont à la même profondeur que la surface pleine déjà dessinée :
			// LEQUAL + un léger décalage vers la caméra évitent le scintillement (z-fighting)
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-1.0f, -1.0f);
			glDepthFunc(GL_LEQUAL);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDepthFunc(GL_LESS);
		}
	}

}
