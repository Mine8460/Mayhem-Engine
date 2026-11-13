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
		enum class CullMode
		{
			None,
			Front,
			Back,
		};
	public:
		virtual void Init() = 0;
		virtual void SetViewport(uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h) = 0;
		virtual void Clear(const glm::vec4& _color) = 0;

		// _firstIndex : premier indice à dessiner (sous-mesh)
		virtual void DrawIndexed(const Ref<VertexArray>& _VA, uint32_t _iCount = 0, uint32_t _firstIndex = 0) = 0;

		inline static API GetAPI() { return s_API;  }
		virtual void SetCullMode(CullMode _mode) = 0;
		// Dessine les triangles en lignes (arêtes), légèrement avancées pour passer devant la surface pleine
		virtual void SetWireframe(bool _enabled) = 0;
	private:
		static API s_API;
	};
}