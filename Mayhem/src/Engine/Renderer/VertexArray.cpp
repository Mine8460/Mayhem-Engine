#include "Enpch.h"
#include "VertexArray.h"

#include <Engine/Renderer/Renderer.h>
#include <Platform/OpenGL/OpenGLVertexArray.h>

namespace Mayhem
{
	Ref<VertexArray> VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLVertexArray>();
		case RendererAPI::API::Direct3D: return nullptr;
		case RendererAPI::API::Vulkan: return nullptr;
		default:
			break;
		}

		return nullptr;
	}
}
