#include "Enpch.h"
#include "SceneRenderer.h"

#include "../Scene/Components.h" // SpriteRenderer, MeshRenderer
#include "Shader.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>

#include <unordered_set>

namespace Mayhem
{
	static const glm::vec3 s_DefaultLightDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.6f));
	static const glm::vec4 s_FullUV = { 0.0f, 0.0f, 1.0f, 1.0f }; // (minU, minV, maxU, maxV)

	struct SceneRendererData
	{
		Ref<Texture2D> WhiteTexture;

		Ref<Mesh> QuadMesh;
		Ref<Mesh> PlaneMesh;
		Ref<Mesh> CubeMesh;

		// État de la scène en cours
		glm::mat4 ViewProjection = glm::mat4(1.0f);
		glm::vec3 CameraPosition = glm::vec3(0.0f);

		glm::vec3 LightDirection = s_DefaultLightDirection; // direction dans laquelle la lumière voyage
		glm::vec3 LightColor = glm::vec3(1.0f);              // couleur * intensité
		glm::vec3 AmbientColor = glm::vec3(0.2f);

		// Shaders qui ont déjà reçu la caméra et la lumière pour cette scène
		std::unordered_set<Shader*> PreparedShaders;

		SceneRenderer::Statistics Stats;
	};

	static SceneRendererData s_Data;

	static glm::mat4 MakeTransform(const glm::vec3& _pos, const glm::vec3& _size)
	{
		return glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), _size);
	}

	static glm::vec4 UVRectFromCoords(const glm::vec2* _tc)
	{
		// Coin 0 = min, coin 2 = max
		return { _tc[0].x, _tc[0].y, _tc[2].x, _tc[2].y };
	}

	void SceneRenderer::Init()
	{
		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data.QuadMesh = Mesh::CreateQuad();
		s_Data.PlaneMesh = Mesh::CreatePlane();
		s_Data.CubeMesh = Mesh::CreateCube();
	}

	void SceneRenderer::Shutdown()
	{
	}

	// ---------- Scène ----------

	void SceneRenderer::SetCamera(const glm::mat4& _viewProj, const glm::vec3& _cameraPos)
	{
		s_Data.ViewProjection = _viewProj;
		s_Data.CameraPosition = _cameraPos;

		// Lumière par défaut si la scène n'a pas de DirectionalLight
		SetDirectionalLightDirection(s_DefaultLightDirection);

		s_Data.PreparedShaders.clear();
	}

	void SceneRenderer::BeginScene(const OrthographicCamera& _cam)
	{
		SetCamera(_cam.GetViewProjectionMatrix(), _cam.GetPosition());
	}

	void SceneRenderer::BeginScene(const EditorCamera& _cam)
	{
		SetCamera(_cam.GetViewProjection(), _cam.GetPosition());
	}

	void SceneRenderer::BeginScene(const Camera& _cam, const glm::mat4& _transform)
	{
		SetCamera(_cam.GetProjection() * glm::inverse(_transform), glm::vec3(_transform[3]));
	}

	void SceneRenderer::EndScene()
	{
		// Rendu immédiat : rien à flush
	}

	// ---------- Lumière ----------

	void SceneRenderer::SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color, float _intensity, float _ambient)
	{
		// w = 0 : on ignore la translation
		glm::vec3 forward = glm::vec3(_transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
		SetDirectionalLightDirection(forward, _color, _intensity, _ambient);
	}

	void SceneRenderer::SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color, float _intensity, float _ambient)
	{
		s_Data.LightDirection = glm::length(_direction) > 0.0001f ? glm::normalize(_direction) : s_DefaultLightDirection;
		s_Data.LightColor = _color * _intensity;
		s_Data.AmbientColor = _color * _ambient;

		// La lumière a changé : tous les shaders doivent la recevoir à nouveau
		s_Data.PreparedShaders.clear();
	}

	// Envoie les uniforms "de scène" (caméra, lumière) une seule fois par shader et par scène
	void SceneRenderer::PrepareShader(const Ref<Shader>& _shader)
	{
		_shader->Bind();

		if (s_Data.PreparedShaders.count(_shader.get()))
			return;

		_shader->SetUniformMat4("u_ViewProjection", s_Data.ViewProjection);
		_shader->SetUniformFloat3("u_CameraPosition", s_Data.CameraPosition);
		_shader->SetUniformFloat3("u_LightDirection", s_Data.LightDirection);
		_shader->SetUniformFloat3("u_LightColor", s_Data.LightColor);
		_shader->SetUniformFloat3("u_AmbientColor", s_Data.AmbientColor);
		_shader->SetUniformInt("u_Texture", 0);

		s_Data.PreparedShaders.insert(_shader.get());
	}

	// ---------- Coeur : un seul chemin de rendu ----------

	void SceneRenderer::Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material,
		const Ref<Texture2D>& _textureOverride, const glm::vec4& _tint, float _tilingMultiplier,
		const glm::vec4& _uvRect, int _entityID)
	{
		if (!_mesh)
			return;

		const Ref<Material>& mat = _material ? _material : Material::GetDefault();
		const Ref<Shader>& shader = mat->GetShader();
		if (!shader)
			return;

		PrepareShader(shader);

		shader->SetUniformMat4("u_Transform", _transform);
		shader->SetUniformFloat4("u_Color", mat->m_AlbedoColor * _tint);
		shader->SetUniformFloat4("u_UVRect", _uvRect);
		shader->SetUniformFloat2("u_Tiling", mat->m_Tiling * _tilingMultiplier);
		shader->SetUniformFloat2("u_Offset", mat->m_Offset);
		shader->SetUniformFloat("u_AlphaCutoff", mat->m_AlphaCutoff);
		shader->SetUniformInt("u_Lit", mat->m_Lit ? 1 : 0);
		shader->SetUniformFloat("u_SpecularStrength", mat->m_SpecularStrength);
		shader->SetUniformFloat("u_Shininess", mat->m_Shininess);
		shader->SetUniformFloat3("u_EmissionColor", mat->m_EmissionColor * mat->m_EmissionIntensity);
		shader->SetUniformInt("u_EntityID", _entityID);

		// Priorité : texture de l'instance (sprite) > texture du matériau > blanc
		Ref<Texture2D> texture = _textureOverride ? _textureOverride : mat->GetAlbedoMap();
		(texture ? texture : s_Data.WhiteTexture)->Bind(0);

		RenderCommand::DrawIndexed(_mesh->GetVertexArray(), _mesh->GetIndexCount());

		s_Data.Stats.DrawCalls++;
	}

	// ---------- Components ----------

	void SceneRenderer::DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, int _entityID)
	{
		// Chargement paresseux (ex : après désérialisation où seul le chemin est connu)
		if (!_src.m_Material && !_src.m_MaterialPath.empty())
			_src.m_Material = Material::Load(_src.m_MaterialPath);

		const Ref<Material>& mat = _src.m_Material ? _src.m_Material : Material::GetDefaultUnlit();

		if (_src.m_Texture)
			Submit(s_Data.QuadMesh, _transform, mat, _src.m_Texture, _src.m_Color, _src.m_Material->m_Tiling.x,
				UVRectFromCoords(_src.m_Texture->GetTextureCoords()), _entityID);
		else
			Submit(s_Data.QuadMesh, _transform, mat, nullptr, _src.m_Color, _src.m_Material->m_Tiling.x, s_FullUV, _entityID);

		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawMeshRenderer(const glm::mat4& _transform, MeshRenderer& _src, int _entityID)
	{
		if (!_src.m_Mesh && !_src.m_MeshPath.empty())
			_src.m_Mesh = Mesh::Create(_src.m_MeshPath);

		if (!_src.m_Material && !_src.m_MaterialPath.empty())
			_src.m_Material = Material::Load(_src.m_MaterialPath);

		if (!_src.m_Mesh)
			return;

		DrawMesh(_src.m_Mesh, _transform, _src.m_Material, _src.m_Color, _entityID);
	}

	// ---------- Mesh / primitives 3D ----------

	void SceneRenderer::DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		Submit(_mesh, _transform, _material ? _material : Material::GetDefault(), nullptr, _tint, 1.0f, s_FullUV, _entityID);
		s_Data.Stats.MeshCount++;
	}

	void SceneRenderer::DrawPlane(const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		DrawMesh(s_Data.PlaneMesh, _transform, _material, _tint, _entityID);
	}

	void SceneRenderer::DrawCube(const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		DrawMesh(s_Data.CubeMesh, _transform, _material, _tint, _entityID);
	}

	void SceneRenderer::DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color)
	{
		DrawCube(MakeTransform(_pos, _size), nullptr, _color);
	}

	// ---------- Quads 2D ----------

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4& _color)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _color);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _color);
	}

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), nullptr, _color, 1.0f, s_FullUV, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), _texture, _color, _tiling, s_FullUV, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), _texture->GetTexture(), _color, _tiling,
			UVRectFromCoords(_texture->GetTextureCoords()), _entityID);
		s_Data.Stats.QuadCount++;
	}

	// ---------- Stats ----------

	SceneRenderer::Statistics SceneRenderer::GetStats()
	{
		return s_Data.Stats;
	}

	void SceneRenderer::ResetStats()
	{
		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.QuadCount = 0;
		s_Data.Stats.MeshCount = 0;
	}
}
