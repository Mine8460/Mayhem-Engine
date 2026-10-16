#include "Enpch.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <iostream>

namespace Mayhem
{
	Ref<Mesh> Mesh::Create(const std::string& _path)
	{
		static std::unordered_map<std::string, Ref<Mesh>> s_Cache;

		auto it = s_Cache.find(_path);
		if (it != s_Cache.end())
			return it->second;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(_path,
			aiProcess_Triangulate |            // tout en triangles
			aiProcess_GenSmoothNormals |       // normales si le fichier n'en a pas
			aiProcess_JoinIdenticalVertices |  // réutilise les sommets identiques (moins de VRAM)
			aiProcess_PreTransformVertices |   // applique la hiérarchie de nodes FBX et fusionne les sous-meshes
			aiProcess_ImproveCacheLocality);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->HasMeshes())
		{
			// Remplace par ton logger (ex: MY_CORE_ERROR)
			std::cerr << "[Mesh] Impossible de charger '" << _path << "' : " << importer.GetErrorString() << std::endl;
			return nullptr;
		}

		// Conversion d'unité : FBX stocke "UnitScaleFactor" (1 = centimètre, 100 = mètre).
		// Le moteur travaille en mètres, donc on ramène tout à 1 unité = 1 m.
		// Les autres formats n'ont pas cette métadonnée : on garde l'échelle telle quelle.
		float unitScaleFactor = 100.0f;
		if (scene->mMetaData)
		{
			double d = 0.0;
			float f = 0.0f;
			if (scene->mMetaData->Get("UnitScaleFactor", d) && d > 0.0)
				unitScaleFactor = (float)d;
			else if (scene->mMetaData->Get("UnitScaleFactor", f) && f > 0.0f)
				unitScaleFactor = f;
		}
		const float scale = unitScaleFactor / 100.0f;

		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;

		// Fusionne tous les sous-meshes du fichier en un seul Mesh
		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			const aiMesh* mesh = scene->mMeshes[m];
			uint32_t baseVertex = (uint32_t)vertices.size();

			for (uint32_t v = 0; v < mesh->mNumVertices; v++)
			{
				MeshVertex vertex;
				vertex.Position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z) * scale;
				vertex.Normal = mesh->HasNormals()
					? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
					: glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.TexCoord = mesh->HasTextureCoords(0)
					? glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
					: glm::vec2(0.0f);
				vertices.push_back(vertex);
			}

			for (uint32_t f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace& face = mesh->mFaces[f];
				if (face.mNumIndices != 3) // ignore points / lignes
					continue;

				indices.push_back(baseVertex + face.mIndices[0]);
				indices.push_back(baseVertex + face.mIndices[1]);
				indices.push_back(baseVertex + face.mIndices[2]);
			}
		}

		Ref<Mesh> result(new Mesh(vertices, indices));
		result->m_Path = _path;
		s_Cache[_path] = result;
		return result;
	}

	Mesh::Mesh(const std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices)
	{
		m_VertexArray = VertexArray::Create();

		// IMPORTANT : on binde NOTRE VAO avant de créer les buffers.
		// Sinon, IndexBuffer::Create fait un glBindBuffer(GL_ELEMENT_ARRAY_BUFFER) pendant que
		// le VAO du dernier mesh dessiné est encore bindé, et lui remplace son index buffer.
		m_VertexArray->Bind();

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

		m_VertexArray->Unbind(); // ne laisse aucun VAO bindé pour le prochain code qui crée des buffers

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
