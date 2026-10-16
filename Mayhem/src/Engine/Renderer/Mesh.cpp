#include "Enpch.h"
#include "Mesh.h"

namespace Mayhem
{
	Mesh::Mesh(const std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices)
	{
		m_VertexArray = VertexArray::Create();

		Ref<VertexBuffer> vb;
		vb.reset(VertexBuffer::Create((float*)_vertices.data(), (uint32_t)(_vertices.size() * sizeof(MeshVertex))));
		vb->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		});
		m_VertexArray->AddVertexBuffer(vb);

		Ref<IndexBuffer> ib;
		ib.reset(IndexBuffer::Create(const_cast<uint32_t*>(_indices.data()), (uint32_t)_indices.size()));
		m_VertexArray->SetIndexBuffer(ib);

		m_IndexCount = (uint32_t)_indices.size();
	}

	// Génère les indices pour une suite de faces de 4 sommets (0,1,2 / 2,3,0)
	static std::vector<uint32_t> MakeQuadIndices(uint32_t _faceCount)
	{
		std::vector<uint32_t> indices;
		indices.reserve(_faceCount * 6);
		for (uint32_t f = 0; f < _faceCount; f++)
		{
			uint32_t o = f * 4;
			indices.insert(indices.end(), { o + 0, o + 1, o + 2, o + 2, o + 3, o + 0 });
		}
		return indices;
	}

	Ref<Mesh> Mesh::CreateQuad()
	{
		std::vector<MeshVertex> v = {
			{ { -0.5f, -0.5f, 0.0f }, { 0, 0, 1 }, { 0, 0 } },
			{ {  0.5f, -0.5f, 0.0f }, { 0, 0, 1 }, { 1, 0 } },
			{ {  0.5f,  0.5f, 0.0f }, { 0, 0, 1 }, { 1, 1 } },
			{ { -0.5f,  0.5f, 0.0f }, { 0, 0, 1 }, { 0, 1 } },
		};
		return Ref<Mesh>(new Mesh(v, MakeQuadIndices(1)));
	}

	Ref<Mesh> Mesh::CreatePlane()
	{
		std::vector<MeshVertex> v = {
			{ { -0.5f, 0.0f,  0.5f }, { 0, 1, 0 }, { 0, 0 } },
			{ {  0.5f, 0.0f,  0.5f }, { 0, 1, 0 }, { 1, 0 } },
			{ {  0.5f, 0.0f, -0.5f }, { 0, 1, 0 }, { 1, 1 } },
			{ { -0.5f, 0.0f, -0.5f }, { 0, 1, 0 }, { 0, 1 } },
		};
		return Ref<Mesh>(new Mesh(v, MakeQuadIndices(1)));
	}

	Ref<Mesh> Mesh::CreateCube()
	{
		std::vector<MeshVertex> v = {
			// Avant (+Z)
			{ { -0.5f, -0.5f,  0.5f }, {  0,  0,  1 }, { 0, 0 } },
			{ {  0.5f, -0.5f,  0.5f }, {  0,  0,  1 }, { 1, 0 } },
			{ {  0.5f,  0.5f,  0.5f }, {  0,  0,  1 }, { 1, 1 } },
			{ { -0.5f,  0.5f,  0.5f }, {  0,  0,  1 }, { 0, 1 } },
			// Arrière (-Z)
			{ {  0.5f, -0.5f, -0.5f }, {  0,  0, -1 }, { 0, 0 } },
			{ { -0.5f, -0.5f, -0.5f }, {  0,  0, -1 }, { 1, 0 } },
			{ { -0.5f,  0.5f, -0.5f }, {  0,  0, -1 }, { 1, 1 } },
			{ {  0.5f,  0.5f, -0.5f }, {  0,  0, -1 }, { 0, 1 } },
			// Gauche (-X)
			{ { -0.5f, -0.5f, -0.5f }, { -1,  0,  0 }, { 0, 0 } },
			{ { -0.5f, -0.5f,  0.5f }, { -1,  0,  0 }, { 1, 0 } },
			{ { -0.5f,  0.5f,  0.5f }, { -1,  0,  0 }, { 1, 1 } },
			{ { -0.5f,  0.5f, -0.5f }, { -1,  0,  0 }, { 0, 1 } },
			// Droite (+X)
			{ {  0.5f, -0.5f,  0.5f }, {  1,  0,  0 }, { 0, 0 } },
			{ {  0.5f, -0.5f, -0.5f }, {  1,  0,  0 }, { 1, 0 } },
			{ {  0.5f,  0.5f, -0.5f }, {  1,  0,  0 }, { 1, 1 } },
			{ {  0.5f,  0.5f,  0.5f }, {  1,  0,  0 }, { 0, 1 } },
			// Haut (+Y)
			{ { -0.5f,  0.5f,  0.5f }, {  0,  1,  0 }, { 0, 0 } },
			{ {  0.5f,  0.5f,  0.5f }, {  0,  1,  0 }, { 1, 0 } },
			{ {  0.5f,  0.5f, -0.5f }, {  0,  1,  0 }, { 1, 1 } },
			{ { -0.5f,  0.5f, -0.5f }, {  0,  1,  0 }, { 0, 1 } },
			// Bas (-Y)
			{ { -0.5f, -0.5f, -0.5f }, {  0, -1,  0 }, { 0, 0 } },
			{ {  0.5f, -0.5f, -0.5f }, {  0, -1,  0 }, { 1, 0 } },
			{ {  0.5f, -0.5f,  0.5f }, {  0, -1,  0 }, { 1, 1 } },
			{ { -0.5f, -0.5f,  0.5f }, {  0, -1,  0 }, { 0, 1 } },
		};
		return Ref<Mesh>(new Mesh(v, MakeQuadIndices(6)));
	}
}
