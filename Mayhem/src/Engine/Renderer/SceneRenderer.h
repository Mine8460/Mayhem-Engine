#pragma once

#include "Camera/OrthographicCamera.h"
#include "Camera/EditorCamera.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/glm.hpp>

namespace Mayhem
{
	struct SpriteRenderer;
	struct MeshRenderer;

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
		// Direction = axe -Z local de la transform, seule la rotation compte.
		static void SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);
		static void SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);

		// ---------- Components ----------
		static void DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, int _entityID = -1);
		static void DrawMeshRenderer(const glm::mat4& _transform, MeshRenderer& _src, int _entityID = -1);

		// ---------- Mesh / primitives 3D (material nullptr = matériau par défaut) ----------
		static void DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawPlane(const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawCube(const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color);

		// ---------- Quads 2D (matériau non éclairé par défaut) ----------
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t MeshCount = 0;
		};

		static Statistics GetStats();
		static void ResetStats();

	private:
		static void SetCamera(const glm::mat4& _viewProj, const glm::vec3& _cameraPos);
		static void PrepareShader(const Ref<Shader>& _shader);

		// Point de passage unique de tous les draws
		static void Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material,
			const Ref<Texture2D>& _textureOverride, const glm::vec4& _tint, float _tilingMultiplier,
			const glm::vec4& _uvRect, int _entityID);
	};
}
