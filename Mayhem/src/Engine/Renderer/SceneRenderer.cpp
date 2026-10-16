#include "Enpch.h"
#include "SceneRenderer.h"

#include "Shader.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Mayhem
{
	struct RendererData
	{
		Ref<Shader> StandardShader;
		Ref<Texture2D> WhiteTexture;

		Ref<Mesh> QuadMesh;
		Ref<Mesh> PlaneMesh;
		Ref<Mesh> CubeMesh;

		// Lumière directionnelle courante
		glm::vec3 LightDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.6f)); // direction dans laquelle la lumière voyage
		glm::vec3 LightColor = glm::vec3(1.0f);  // couleur * intensité
		glm::vec3 AmbientColor = glm::vec3(0.2f);

		SceneRenderer::Statistics Stats;
	};

	static const glm::vec3 s_DefaultLightDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.6f));

	static RendererData s_Data;

	static const glm::vec4 s_FullUV = { 0.0f, 0.0f, 1.0f, 1.0f }; // (minU, minV, maxU, maxV)

	static glm::mat4 MakeTransform(const glm::vec3& _pos, const glm::vec3& _size)
	{
		return glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), _size);
	}

	void SceneRenderer::Init()
	{
		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data.QuadMesh = Mesh::CreateQuad();
		s_Data.PlaneMesh = Mesh::CreatePlane();
		s_Data.CubeMesh = Mesh::CreateCube();

		s_Data.StandardShader = Shader::Create("assets/shaders/Standard.glsl");
		s_Data.StandardShader->Bind();
		s_Data.StandardShader->SetUniformInt("u_Texture", 0);
	}

	void SceneRenderer::Shutdown()
	{
	}

	// ---------- Scène ----------

	void SceneRenderer::SetViewProjection(const glm::mat4& _viewProj)
	{
		s_Data.StandardShader->Bind();
		s_Data.StandardShader->SetUniformMat4("u_ViewProjection", _viewProj);

		// Lumière par défaut si la scène n'a pas de DirectionalLight
		SetDirectionalLightDirection(s_DefaultLightDirection);
	}

	// ---------- Lumière ----------

	void SceneRenderer::UploadLight()
	{
		s_Data.StandardShader->Bind();
		s_Data.StandardShader->SetUniformFloat3("u_LightDirection", s_Data.LightDirection);
		s_Data.StandardShader->SetUniformFloat3("u_LightColor", s_Data.LightColor);
		s_Data.StandardShader->SetUniformFloat3("u_AmbientColor", s_Data.AmbientColor);
	}

	void SceneRenderer::SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color, float _intensity, float _ambient)
	{
		// w = 0 : on ignore la translation, seule la rotation oriente la lumière
		glm::vec3 forward = glm::vec3(_transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
		SetDirectionalLightDirection(forward, _color, _intensity, _ambient);
	}

	void SceneRenderer::SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color, float _intensity, float _ambient)
	{
		s_Data.LightDirection = glm::length(_direction) > 0.0001f ? glm::normalize(_direction) : s_DefaultLightDirection;
		s_Data.LightColor = _color * _intensity;
		s_Data.AmbientColor = _color * _ambient;
		UploadLight();
	}

	void SceneRenderer::BeginScene(const OrthographicCamera& _cam)
	{
		SetViewProjection(_cam.GetViewProjectionMatrix());
	}

	void SceneRenderer::BeginScene(const EditorCamera& _cam)
	{
		SetViewProjection(_cam.GetViewProjection());
	}

	void SceneRenderer::BeginScene(const Camera& _cam, const glm::mat4& _transform)
	{
		SetViewProjection(_cam.GetProjection() * glm::inverse(_transform));
	}

	void SceneRenderer::EndScene()
	{
		// Rendu immédiat : rien à flush
	}

	// ---------- Coeur : un seul chemin de rendu ----------

	void SceneRenderer::Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Texture2D>& _texture,
		const glm::vec4& _color, float _tiling, const glm::vec4& _uvRect, bool _lit, int _entityID)
	{
		s_Data.StandardShader->Bind();
		s_Data.StandardShader->SetUniformMat4("u_Transform", _transform);
		s_Data.StandardShader->SetUniformFloat4("u_Color", _color);
		s_Data.StandardShader->SetUniformFloat4("u_UVRect", _uvRect);
		s_Data.StandardShader->SetUniformFloat("u_TilingFactor", _tiling);
		s_Data.StandardShader->SetUniformInt("u_Lit", _lit ? 1 : 0);
		s_Data.StandardShader->SetUniformInt("u_EntityID", _entityID);

		(_texture ? _texture : s_Data.WhiteTexture)->Bind(0);

		RenderCommand::DrawIndexed(_mesh->GetVertexArray(), _mesh->GetIndexCount());

		s_Data.Stats.DrawCalls++;
	}

	// ---------- Mesh ----------

	void SceneRenderer::DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		Submit(_mesh, _transform, s_Data.WhiteTexture, _color, 1.0f, s_FullUV, true, _entityID);
		s_Data.Stats.MeshCount++;
	}

	void SceneRenderer::DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(_mesh, _transform, _texture, _color, _tiling, s_FullUV, true, _entityID);
		s_Data.Stats.MeshCount++;
	}

	// ---------- Quad ----------

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
		Submit(s_Data.QuadMesh, _transform, s_Data.WhiteTexture, _color, 1.0f, s_FullUV, false, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, _texture, _color, _tiling, s_FullUV, false, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		// Les coords de la SubTexture deviennent un rectangle UV (coin 0 = min, coin 2 = max)
		const glm::vec2* tc = _texture->GetTextureCoords();
		glm::vec4 uvRect = { tc[0].x, tc[0].y, tc[2].x, tc[2].y };

		Submit(s_Data.QuadMesh, _transform, _texture->GetTexture(), _color, _tiling, uvRect, false, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, unsigned int _entityID)
	{
		if (_src.m_Texture)
		{
			const glm::vec2* tc = _src.m_Texture->GetTextureCoords();
			glm::vec4 uvRect = { tc[0].x, tc[0].y, tc[2].x, tc[2].y };
			Submit(s_Data.QuadMesh, _transform, _src.m_Texture, _src.m_Color, _src.m_Tiling, uvRect, false, (int)_entityID);
		}
		else
		{
			Submit(s_Data.QuadMesh, _transform, s_Data.WhiteTexture, _src.m_Color, _src.m_Tiling, s_FullUV, false, (int)_entityID);
		}
		s_Data.Stats.QuadCount++;
	}

	// ---------- Plane ----------

	void SceneRenderer::DrawPlane(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color)
	{
		DrawPlane(MakeTransform(_pos, { _size.x, 1.0f, _size.y }), _color);
	}

	void SceneRenderer::DrawPlane(const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		DrawMesh(s_Data.PlaneMesh, _transform, _color, _entityID);
	}

	void SceneRenderer::DrawPlane(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		DrawMesh(s_Data.PlaneMesh, _transform, _texture, _color, _tiling, _entityID);
	}

	// ---------- Cube ----------

	void SceneRenderer::DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color)
	{
		DrawCube(MakeTransform(_pos, _size), _color);
	}

	void SceneRenderer::DrawCube(const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		DrawMesh(s_Data.CubeMesh, _transform, _color, _entityID);
	}

	void SceneRenderer::DrawCube(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		DrawMesh(s_Data.CubeMesh, _transform, _texture, _color, _tiling, _entityID);
	}

	void SceneRenderer::DrawMeshRenderer(const glm::mat4& _transform, MeshRenderer& _src, int _entityID)
	{
		// Chargement paresseux (ex : après désérialisation où seul le chemin est connu)
		if (!_src.m_Mesh && !_src.m_MeshPath.empty())
			_src.m_Mesh = Mesh::Create(_src.m_MeshPath);

		if (!_src.m_Mesh)
			return;

		DrawMesh(_src.m_Mesh, _transform, _src.m_Texture, _src.m_Color, _src.m_Tiling, _entityID);
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
