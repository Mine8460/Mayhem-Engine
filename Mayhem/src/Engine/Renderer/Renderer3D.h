#pragma once

#include <Engine/Renderer/Camera/EditorCamera.h>

namespace Mayhem
{
	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const EditorCamera& _cam);
		static void EndScene();

		static void DrawCube(const glm::mat4& _transform, const glm::vec4& _color);
	};
}