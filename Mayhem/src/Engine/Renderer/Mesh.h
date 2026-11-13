#pragma once

#include "VertexArray.h"
#include "Frustum.h"
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
		glm::vec4 Tangent = glm::vec4(0.0f); // xyz = tangente (sens +U), w = signe de la bitangente (UV en miroir)
	};

	// Une plage d'indices dessinée avec un seul matériau (un "slot" de matériau)
	struct SubMesh
	{
		uint32_t BaseIndex = 0;     // premier indice dans l'index buffer
		uint32_t IndexCount = 0;
		uint32_t MaterialIndex = 0; // slot de matériau du MeshRenderer
		AABB Bounds;                // boîte englobante locale de cette partie (frustum culling)
	};

	class Mesh
	{
	public:
		// _subMeshes vide = un seul sous-mesh qui couvre tout, avec un seul slot de matériau
		Mesh(const std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices,
			const std::vector<SubMesh>& _subMeshes = {}, const std::vector<std::string>& _materialSlotNames = {});

		const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }
		uint32_t GetIndexCount() const { return m_IndexCount; }
		const std::string& GetPath() const { return m_Path; }
		const AABB& GetBounds() const { return m_Bounds; } // boîte englobante en espace local

		// ---------- Slots de matériaux ----------
		// Un fichier 3D avec plusieurs matériaux est découpé en sous-meshes (un par matériau du fichier).
		const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		uint32_t GetMaterialSlotCount() const { return (uint32_t)m_MaterialSlotNames.size(); }
		const std::string& GetMaterialSlotName(uint32_t _slot) const { return m_MaterialSlotNames[_slot]; }

		// Calcule les tangentes (normal mapping) à partir des positions / UV / normales
		static void ComputeTangents(std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices);

		// Charge un fichier 3D (.fbx, .obj, .gltf…) via Assimp.
		// Mis en cache : le même chemin renvoie le même Mesh. Renvoie nullptr en cas d'échec.
		static Ref<Mesh> Create(const std::string& _path);

		// Primitives (taille 1, centrées sur l'origine)
		static Ref<Mesh> CreateQuad();  // plan XY, face vers +Z (2D / sprites)
		static Ref<Mesh> CreatePlane(); // plan XZ, face vers +Y (sol)
		static Ref<Mesh> CreateCube();
		static Ref<Mesh> CreateSphere(uint32_t _segments = 48, uint32_t _rings = 24); // rayon 0.5

		// Mesh déjà chargé en mémoire (nullptr sinon) : ne lit jamais le disque
		static Ref<Mesh> GetLoaded(const std::string& _path);

	private:
		Ref<VertexArray> m_VertexArray;
		uint32_t m_IndexCount = 0;
		std::string m_Path; // vide pour les primitives
		AABB m_Bounds;
		std::vector<SubMesh> m_SubMeshes;
		std::vector<std::string> m_MaterialSlotNames;
	};
}
