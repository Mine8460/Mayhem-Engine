#pragma once

#include "VertexArray.h"
#include <glm/glm.hpp>
#include <vector>

namespace Mayhem
{
	struct MeshVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

	class Mesh
	{
	public:
		Mesh(const std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices);

		const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }
		uint32_t GetIndexCount() const { return m_IndexCount; }

		// Primitives (taille 1, centrées sur l'origine)
		static Ref<Mesh> CreateQuad();  // plan XY, face vers +Z (2D / sprites)
		static Ref<Mesh> CreatePlane(); // plan XZ, face vers +Y (sol)
		static Ref<Mesh> CreateCube();

	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount = 0;
	};
}
