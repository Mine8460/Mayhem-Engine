#include "Enpch.h"
#include "Renderer3D.h"

#include "VertexArray.h"
#include "Shader.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Mayhem
{
	struct CubeVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

	struct Renderer3DData
	{
		static const uint32_t CubeVertexCount = 24; // 4 sommets par face (normales/UV propres à chaque face)
		static const uint32_t CubeIndexCount = 36;  // 6 faces * 2 triangles * 3

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> CubeShader;
		Ref<Texture2D> WhiteTexture;

		Renderer3D::Statistics Stats;
	};

	static Renderer3DData s_Data;

	void Renderer3D::Init()
	{
		s_Data.CubeVA = VertexArray::Create();

		// Cube unitaire centré sur l'origine, faces en sens anti-horaire vues de l'extérieur
		CubeVertex cubeVertices[Renderer3DData::CubeVertexCount] = {
			// Avant (+Z)
			{ { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
			// Arrière (-Z)
			{ {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
			{ { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
			// Gauche (-X)
			{ { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
			{ { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
			{ { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
			// Droite (+X)
			{ {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
			// Haut (+Y)
			{ { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
			// Bas (-Y)
			{ { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
		};

		s_Data.CubeVB.reset(VertexBuffer::Create((float*)cubeVertices, sizeof(cubeVertices)));

		BufferLayout layout =
		{
			{ ShaderDataType::Float3,	"a_Position"},
			{ ShaderDataType::Float3,	"a_Normal"},
			{ ShaderDataType::Float2,	"a_TexCoord"},
		};
		s_Data.CubeVB->SetLayout(layout);
		s_Data.CubeVA->AddVertexBuffer(s_Data.CubeVB);

		uint32_t cubeIndices[Renderer3DData::CubeIndexCount];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < Renderer3DData::CubeIndexCount; i += 6)
		{
			cubeIndices[i + 0] = offset + 0;
			cubeIndices[i + 1] = offset + 1;
			cubeIndices[i + 2] = offset + 2;

			cubeIndices[i + 3] = offset + 2;
			cubeIndices[i + 4] = offset + 3;
			cubeIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> cubeIB;
		cubeIB.reset(IndexBuffer::Create(cubeIndices, Renderer3DData::CubeIndexCount));
		s_Data.CubeVA->SetIndexBuffer(cubeIB);

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data.CubeShader = Shader::Create("assets/shaders/Cube.glsl");
		s_Data.CubeShader->Bind();
		s_Data.CubeShader->SetUniformInt("u_Texture", 0);
	}

	void Renderer3D::Shutdown()
	{
	}

	void Renderer3D::SetViewProjection(const glm::mat4& _viewProj)
	{
		s_Data.CubeShader->Bind();
		s_Data.CubeShader->SetUniformMat4("u_ViewProjection", _viewProj);
	}

	void Renderer3D::BeginScene(const OrthographicCamera& _cam)
	{
		SetViewProjection(_cam.GetViewProjectionMatrix());
	}

	void Renderer3D::BeginScene(const EditorCamera& _cam)
	{
		SetViewProjection(_cam.GetViewProjection());
	}

	void Renderer3D::BeginScene(const Camera& _cam, const glm::mat4& _transform)
	{
		SetViewProjection(_cam.GetProjection() * glm::inverse(_transform));
	}

	void Renderer3D::EndScene()
	{
		// Plus de batch : chaque cube est dessiné immédiatement dans DrawCube
	}

	void Renderer3D::Submit(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		s_Data.CubeShader->Bind();
		s_Data.CubeShader->SetUniformMat4("u_Transform", _transform);
		s_Data.CubeShader->SetUniformFloat4("u_Color", _color);
		s_Data.CubeShader->SetUniformFloat("u_TilingFactor", _tiling);
		s_Data.CubeShader->SetUniformInt("u_EntityID", _entityID);

		_texture->Bind(0);

		RenderCommand::DrawIndexed(s_Data.CubeVA, Renderer3DData::CubeIndexCount);

		s_Data.Stats.DrawCalls++;
		s_Data.Stats.CubeCount++;
	}

	// ---------- Couleur unie ----------

	void Renderer3D::DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), _pos)
			* glm::scale(glm::mat4(1.0f), _size);

		DrawCube(transform, _color);
	}

	void Renderer3D::DrawCube(const glm::vec3& _pos, const glm::vec3& _rotation, const glm::vec3& _size, const glm::vec4& _color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), _pos)
			* glm::toMat4(glm::quat(_rotation)) // rotation en radians (x, y, z)
			* glm::scale(glm::mat4(1.0f), _size);

		DrawCube(transform, _color);
	}

	void Renderer3D::DrawCube(const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		Submit(_transform, s_Data.WhiteTexture, _color, 1.0f, _entityID);
	}

	// ---------- Texturé ----------

	void Renderer3D::DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), _pos)
			* glm::scale(glm::mat4(1.0f), _size);

		DrawCube(transform, _texture, _color, _tiling);
	}

	void Renderer3D::DrawCube(const glm::vec3& _pos, const glm::vec3& _rotation, const glm::vec3& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), _pos)
			* glm::toMat4(glm::quat(_rotation))
			* glm::scale(glm::mat4(1.0f), _size);

		DrawCube(transform, _texture, _color, _tiling);
	}

	void Renderer3D::DrawCube(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(_transform, _texture ? _texture : s_Data.WhiteTexture, _color, _tiling, _entityID);
	}

	// ---------- Stats ----------

	Renderer3D::Statistics Renderer3D::GetStats()
	{
		return s_Data.Stats;
	}

	void Renderer3D::ResetStats()
	{
		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.CubeCount = 0;
	}
}
