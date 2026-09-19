#include <Enpch.h>
#include "Renderer.h"
#include "Renderer2D.h"
#include <Platform/OpenGL/OpenGLShader.h>

namespace Mayhem
{
	Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData;

	void Renderer::Init()
	{
		RenderCommand::Init();
		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t _w, uint32_t _h)
	{
		RenderCommand::SetViewport(0, 0, _w, _h);
	}

	void Renderer::BeginScene(OrthographicCamera& _cam)
	{
		s_SceneData->ViewProjectionMatrix = _cam.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{

	}

	void Renderer::Submit(const Ref<VertexArray>& _VA, const Ref<Shader>& _shader, const glm::mat4& _transform)
	{
		_shader->Bind();

		std::dynamic_pointer_cast<OpenGLShader>(_shader)->UploadUniformMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(_shader)->UploadUniformMat4("u_Transform", _transform);

		_VA->Bind();
		RenderCommand::DrawIndexed(_VA);
	}
}