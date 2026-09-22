#pragma once

#include "Camera/OrthographicCamera.h"
#include "Camera/EditorCamera.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include <Engine/Scene/Components.h>

namespace Mayhem
{
	class Renderer2D
	{
	public:

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};

		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& _cam); // TODO: REMOVE
		static void BeginScene(const EditorCamera& _cam);
		static void BeginScene(const Camera& _cam, const glm::mat4& _transform);
		static void EndScene();
		static void Flush();

		// Primitives
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4 _color);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4 _color);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
	
		static void DrawQuad(const glm::mat4& _transform, const glm::vec4 _color);
		static void DrawQuad(const glm::mat4& _transform, const Ref<Texture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
		static void DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D> _texture, const glm::vec4& _color = { 1.0f, 1.0f, 1.0f, 1.0f }, float _tiling = 1.0f);
	
		static void DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, unsigned int _entityID);

		static Statistics GetStats(void);
		static void ResetStats();

	private:
		static void ResetBatchCount(void);
	};
}
