#pragma once

#include <Engine/Renderer/RendererAPI.h>

namespace Mayhem
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h) override;
		virtual void Clear(const glm::vec4& _color) override;

		virtual void DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount = 0) override;
	};
}
