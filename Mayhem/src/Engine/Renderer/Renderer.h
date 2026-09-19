#pragma once

#include "RenderCommand.h"

#include "Camera/OrthographicCamera.h"
#include "Shader.h"

namespace Mayhem
{

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		static void OnWindowResize(uint32_t _w, uint32_t _h);

		static void BeginScene(OrthographicCamera& _cam); // TODO
		static void EndScene();
		static void Submit(const Ref<VertexArray>& _VA, const Ref<Shader>& _shader, const glm::mat4& _transform = glm::mat4(1.0f));

		inline static RendererAPI::API GetAPI(void) { return RendererAPI::GetAPI();  }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* s_SceneData;
	};

}
