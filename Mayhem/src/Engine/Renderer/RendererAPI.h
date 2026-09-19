#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <Engine/Renderer/VertexArray.h>

namespace Mayhem
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL,
			Direct3D,
			Vulkan
		};
	public:
		virtual void Init() = 0;
		virtual void SetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h) = 0;
		virtual void Clear(const glm::vec4& _color) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount = 0) = 0;

		inline static API GetAPI() { return s_API;  }
	private:
		static API s_API;
	};
}