#include "Enpch.h"
#include "FrameBuffer.h"

#include <Engine/Renderer/RendererAPI.h>
#include <Platform/OpenGL/OpenGLFrameBuffer.h>

namespace Mayhem
{

Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification& _spec)
{
	switch (RendererAPI::GetAPI())
	{
	case RendererAPI::API::None: return nullptr;
	case RendererAPI::API::OpenGL: return MakeRef<OpenGLFrameBuffer>(_spec);
	case RendererAPI::API::Vulkan: return nullptr;
	case RendererAPI::API::Direct3D: return nullptr;
	}
	return nullptr;
}

}
