#pragma once
#include "Camera/OrthographicCamera.h"
#include "Camera/EditorCamera.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include "Mesh.h"
#include "../Scene/Components.h" // SpriteRenderer

#include <glm/glm.hpp>

namespace Mayhem
{
	class SceneRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& _cam);
		static void BeginScene(const EditorCamera& _cam);
		static void BeginScene(const Camera& _cam, const glm::mat4& _transform);
		static void EndScene();

		// ---------- Lumière ----------
		// À appeler APRÈS BeginScene (BeginScene remet la lumière par défaut).
		// La direction est l'axe -Z local de la transform (comme une caméra) : seule la rotation compte.
		static void SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);
		static void SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);

		// ---------- Mesh (base de tout) ----------
		static void DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const glm::vec4& _color = glm::vec4(1.0f), int _entityID = -1);
		static void DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		// ---------- Quad (2D, plan XY, non éclairé) ----------
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		static void DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, unsigned int _entityID);

		// ---------- Plane (3D, plan XZ, éclairé) ----------
		static void DrawPlane(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawPlane(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);
		static void DrawPlane(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		// ---------- Cube ----------
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color);
		static void DrawCube(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);
		static void DrawCube(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t MeshCount = 0;
		};

		static Statistics GetStats();
		static void ResetStats();

	private:
		static void SetViewProjection(const glm::mat4& _viewProj);
		static void UploadLight();
		static void Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Texture2D>& _texture,
			const glm::vec4& _color, float _tiling, const glm::vec4& _uvRect, bool _lit, int _entityID);
	};
}
