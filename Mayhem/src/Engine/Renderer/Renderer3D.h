#pragma once

#include "Camera/OrthographicCamera.h"
#include "Camera/EditorCamera.h"
#include "Texture.h"

#include <glm/glm.hpp>

namespace Mayhem
{
	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& _cam);
		static void BeginScene(const EditorCamera& _cam);
		static void BeginScene(const Camera& _cam, const glm::mat4& _transform);
		static void EndScene();

		// Couleur unie
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color);
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _rotation, const glm::vec3& _size, const glm::vec4& _color);
		static void DrawCube(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);

		// Texturé
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _rotation, const glm::vec3& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawCube(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;

			uint32_t GetTotalVertexCount() const { return CubeCount * 24; }
			uint32_t GetTotalIndexCount() const { return CubeCount * 36; }
		};

		static Statistics GetStats();
		static void ResetStats();

	private:
		static void SetViewProjection(const glm::mat4& _viewProj);
		static void Submit(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID);
	};
}
