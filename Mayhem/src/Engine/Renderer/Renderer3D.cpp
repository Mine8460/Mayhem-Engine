#include "Enpch.h"
#include "Renderer3D.h"

#include "Shader.h"
#include "Texture.h"
#include <array>
#include "Buffer.h"
#include "VertexArray.h"
#include "RenderCommand.h"

namespace Mayhem
{
	struct MeshVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		//// Editor-only
		//int EntityID = 0;
	};

	struct Renderer3DData
	{
		static const uint32_t MaxTexturesSlots = 32; // TODO : RenderCaps

		Ref<VertexArray> MeshVA;
		Ref<VertexBuffer> MeshVB;
		Ref<Shader> Shader;
		Ref<Texture2D> WhiteTexture;

		std::array<Ref<Texture2D>, MaxTexturesSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = White Texture

		MeshVertex* MeshVertexBufferBase = nullptr;
		MeshVertex* MeshVertexBufferPtr = nullptr;
		uint32_t MaxVertices = 1000;
	};

	static Renderer3DData s_Data;

	void Renderer3D::Init()
	{
		// Create vertex array
		s_Data.MeshVA = VertexArray::Create();

		// Create Cube
		glm::vec3 vertices[] =
		{
			{ -0.5f, -0.5f, -0.5f },
			{  0.5f, -0.5f, -0.5f },
			{  0.5f,  0.5f, -0.5f },
			{ -0.5f,  0.5f, -0.5f },

			{ -0.5f, -0.5f,  0.5f },
			{  0.5f, -0.5f,  0.5f },
			{  0.5f,  0.5f,  0.5f },
			{ -0.5f,  0.5f,  0.5f }
		};

		uint32_t indices[] =
		{
			// Back
			0, 1, 2,
			2, 3, 0,

			// Front
			4, 5, 6,
			6, 7, 4,

			// Left
			0, 4, 7,
			7, 3, 0,

			// Right
			1, 5, 6,
			6, 2, 1,

			// Bottom
			0, 1, 5,
			5, 4, 0,

			// Top
			3, 2, 6,
			6, 7, 3
		};

		s_Data.MeshVB.reset(VertexBuffer::Create(sizeof(vertices)));
		s_Data.MeshVB->SetData(vertices, sizeof(vertices));
		BufferLayout layout =
		{
			{ ShaderDataType::Float3,	"a_Position"},
			{ ShaderDataType::Float4,	"a_Color"},
			{ ShaderDataType::Float2,	"a_TexCoord"},
			{ ShaderDataType::Float,	"a_TexIndex"},
			{ ShaderDataType::Float,	"a_TilingFactor"},
			//{ ShaderDataType::Int,		"a_EntityID"},
		};

		s_Data.MeshVB->SetLayout(layout);
		s_Data.MeshVA->AddVertexBuffer(s_Data.MeshVB);

		Ref<IndexBuffer> quadIB;
		quadIB.reset(IndexBuffer::Create(indices, 36));
		s_Data.MeshVA->SetIndexBuffer(quadIB);

		// Create default texture
		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// Create shader
		int samplers[s_Data.MaxTexturesSlots];
		for (uint32_t i = 0; i < s_Data.MaxTexturesSlots; i++)
			samplers[i] = i;

		s_Data.Shader = Shader::Create("assets/shaders/Texture.glsl");
		s_Data.Shader->Bind();
		s_Data.Shader->SetUniformIntArray("u_Texture", samplers, s_Data.MaxTexturesSlots);

		s_Data.MeshVertexBufferBase = new MeshVertex[s_Data.MaxVertices];
		s_Data.MeshVertexBufferPtr = s_Data.MeshVertexBufferBase;
	}

	void Renderer3D::Shutdown()
	{
	}

	void Renderer3D::BeginScene(const EditorCamera& _cam)
	{
		s_Data.Shader->Bind();
		s_Data.Shader->SetUniformMat4("u_ViewProjection", _cam.GetViewProjection());

		//ResetBatchCount();
	}

	void Renderer3D::EndScene()
	{
	}

	void Renderer3D::DrawMesh(const glm::mat4& _transform, const glm::vec4& _color)
	{
		s_Data.Shader->Bind();
		
		s_Data.MeshVertexBufferPtr = s_Data.MeshVertexBufferBase;

		s_Data.MeshVertexBufferPtr->Position = _transform[3];

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.MeshVertexBufferPtr - (uint8_t*)s_Data.MeshVertexBufferBase);
		s_Data.MeshVB->SetData(s_Data.MeshVertexBufferBase, dataSize);

		RenderCommand::DrawIndexed(s_Data.MeshVA);
	}
}
