#include <Enpch.h>
#include "RenderCommand.h"

#include <Platform/OpenGL/OpenGLRendererAPI.h>

namespace Mayhem
{
	RendererAPI* RenderCommand::s_API = new OpenGLRendererAPI;
}