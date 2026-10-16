#pragma once
#include "../Core/Core.h"
#include "VertexArray.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

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
		const std::string& GetPath() const { return m_Path; }

		// Charge un fichier 3D (.fbx, .obj, .gltf…) via Assimp.
		// Mis en cache : le même chemin renvoie le même Mesh. Renvoie nullptr en cas d'échec.
		static Ref<Mesh> Create(const std::string& _path);

		// Primitives (taille 1, centrées sur l'origine)
		static Ref<Mesh> CreateQuad();  // plan XY, face vers +Z (2D / sprites)
		static Ref<Mesh> CreatePlane(); // plan XZ, face vers +Y (sol)
		static Ref<Mesh> CreateCube();

	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount = 0;
		std::string m_Path; // vide pour les primitives
	};
}
