#include "Enpch.h"
#include "Buffer.h"

#include <Engine/Renderer/Renderer.h>

#include <Platform/OpenGL/OpenGLBuffer.h>

namespace Mayhem
{
	VertexBuffer* VertexBuffer::Create(uint32_t _size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return new OpenGLVertexBuffer(_size);
		case RendererAPI::API::Direct3D: return nullptr;
		case RendererAPI::API::Vulkan: return nullptr;
		default:
			break;
		}

		return nullptr;
	}

	VertexBuffer* VertexBuffer::Create(float* _vertices, uint32_t _size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return new OpenGLVertexBuffer(_vertices, _size);
		case RendererAPI::API::Direct3D: return nullptr;
		case RendererAPI::API::Vulkan: return nullptr;
		default:
			break;
		}

		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* _indices, uint32_t _count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return new OpenGLIndexBuffer(_indices, _count);
		case RendererAPI::API::Direct3D: return nullptr;
		case RendererAPI::API::Vulkan: return nullptr;
		default:
			break;
		}

		return nullptr;
	}
}
