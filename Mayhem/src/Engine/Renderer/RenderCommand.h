#pragma once

#include "RendererAPI.h"

namespace Mayhem
{
	class RenderCommand
	{
	public:
		inline static void Init()
		{
			s_API->Init();
		}
		inline static void SetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h)
		{
			s_API->SetViewport(_x, _y, _w, _h);
		}
		inline static void Clear(const glm::vec4& _color)
		{
			s_API->Clear(_color);
		}
		inline static void DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount = 0)
		{
			s_API->DrawIndexed(_VA, _iCount);
		}
	private:
		static RendererAPI* s_API;
	};
}