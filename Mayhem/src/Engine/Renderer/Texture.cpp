#include "Enpch.h"
#include "Texture.h"

#include <Engine/Renderer/RendererAPI.h>
#include <Platform/OpenGL/OpenGLTexture.h>

namespace Mayhem
{
	Ref<Texture2D> Texture2D::Create(uint32_t _w, uint32_t _h)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return  std::make_shared<OpenGLTexture2D>(_w, _h);
		case RendererAPI::API::Vulkan: return nullptr;
		case RendererAPI::API::Direct3D: return nullptr;
		}
		return nullptr;
	}
	Ref<Texture2D> Texture2D::Create(const std::string& _path)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return  std::make_shared<OpenGLTexture2D>(_path);
		case RendererAPI::API::Vulkan: return nullptr;
		case RendererAPI::API::Direct3D: return nullptr;
		}
		return nullptr;
	}
}
