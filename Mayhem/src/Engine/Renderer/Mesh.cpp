#include "Enpch.h"
#include "Mesh.h"

#include <Engine/Core/AssetsManager.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cmath>

namespace Mayhem
{
	// =====================================================================
	//  Cache disque (comme le dossier Library d'Unity)
	//  Le premier import passe par Assimp (lent sur un gros FBX), puis le résultat est écrit
	//  en binaire brut dans Library/Meshes/<id>.mesh : les chargements suivants sont quasi instantanés.
	//  Le cache est refait automatiquement si le fichier source est plus récent.
	// =====================================================================

	static constexpr uint32_t s_CookedMagic = 0x48534D4D; // "MMSH"
	static constexpr uint32_t s_CookedVersion = 2;        // à incrémenter si l'import change (lissage, échelle, format de sommet...)
	                                                      // v2 : tangentes + sous-meshes / slots de matériaux

	struct CookedHeader
	{
		uint32_t Magic = s_CookedMagic;
		uint32_t Version = s_CookedVersion;
		uint64_t VertexCount = 0;
		uint64_t IndexCount = 0;
		uint64_t SubMeshCount = 0;
		uint64_t SlotCount = 0;
	};

	// Résultat d'un import (Assimp ou cache)
	struct MeshData
	{
		std::vector<MeshVertex> Vertices;
		std::vector<uint32_t> Indices;
		std::vector<SubMesh> SubMeshes;
		std::vector<std::string> SlotNames;
	};

	static std::filesystem::path GetCookedPath(AssetID _id)
	{
		return std::filesystem::path("Library") / "Meshes" / (std::to_string(_id) + ".mesh");
	}

	// Cache utilisable : existe, plus récent que la source, bon format
	static bool ReadCooked(const std::filesystem::path& _cooked, const std::string& _source, MeshData& _data)
	{
		std::error_code error;
		if (!std::filesystem::exists(_cooked, error))
			return false;

		auto cookedTime = std::filesystem::last_write_time(_cooked, error);
		auto sourceTime = std::filesystem::last_write_time(_source, error);
		if (error || cookedTime < sourceTime)
			return false;

		std::ifstream file(_cooked, std::ios::binary);
		CookedHeader header;
		if (!file.read((char*)&header, sizeof(header)) || header.Magic != s_CookedMagic || header.Version != s_CookedVersion)
			return false;

		_data.Vertices.resize((size_t)header.VertexCount);
		_data.Indices.resize((size_t)header.IndexCount);
		file.read((char*)_data.Vertices.data(), (std::streamsize)(_data.Vertices.size() * sizeof(MeshVertex)));
		file.read((char*)_data.Indices.data(), (std::streamsize)(_data.Indices.size() * sizeof(uint32_t)));

		_data.SubMeshes.resize((size_t)header.SubMeshCount);
		for (SubMesh& sub : _data.SubMeshes)
		{
			file.read((char*)&sub.BaseIndex, sizeof(uint32_t));
			file.read((char*)&sub.IndexCount, sizeof(uint32_t));
			file.read((char*)&sub.MaterialIndex, sizeof(uint32_t));
		}

		_data.SlotNames.resize((size_t)header.SlotCount);
		for (std::string& name : _data.SlotNames)
		{
			uint32_t length = 0;
			file.read((char*)&length, sizeof(uint32_t));
			if (!file || length > 4096)
				return false;
			name.resize(length);
			file.read(name.data(), length);
		}

		if (!file)
			return false;

		// Sécurité : un cache corrompu ne doit pas faire dessiner hors de l'index buffer
		for (const SubMesh& sub : _data.SubMeshes)
			if ((uint64_t)sub.BaseIndex + sub.IndexCount > _data.Indices.size() || sub.MaterialIndex >= _data.SlotNames.size())
				return false;

		return true;
	}

	static void WriteCooked(const std::filesystem::path& _cooked, const MeshData& _data)
	{
		std::error_code error;
		std::filesystem::create_directories(_cooked.parent_path(), error);

		std::ofstream file(_cooked, std::ios::binary | std::ios::trunc);
		CookedHeader header;
		header.VertexCount = _data.Vertices.size();
		header.IndexCount = _data.Indices.size();
		header.SubMeshCount = _data.SubMeshes.size();
		header.SlotCount = _data.SlotNames.size();
		file.write((const char*)&header, sizeof(header));
		file.write((const char*)_data.Vertices.data(), (std::streamsize)(_data.Vertices.size() * sizeof(MeshVertex)));
		file.write((const char*)_data.Indices.data(), (std::streamsize)(_data.Indices.size() * sizeof(uint32_t)));

		for (const SubMesh& sub : _data.SubMeshes)
		{
			file.write((const char*)&sub.BaseIndex, sizeof(uint32_t));
			file.write((const char*)&sub.IndexCount, sizeof(uint32_t));
			file.write((const char*)&sub.MaterialIndex, sizeof(uint32_t));
		}

		for (const std::string& name : _data.SlotNames)
		{
			uint32_t length = (uint32_t)name.size();
			file.write((const char*)&length, sizeof(uint32_t));
			file.write(name.data(), length);
		}
	}

	// Import complet via Assimp (lent)
	static bool ImportWithAssimp(const std::string& _path, MeshData& _data)
	{
		Assimp::Importer importer;

		// Lissage : on jette les normales du fichier (souvent "flat", une par face)
		// et Assimp les recalcule en les moyennant entre faces voisines.
		// Les arêtes dont l'angle dépasse MaxSmoothingAngle restent nettes
		// (les coins d'une caisse restent durs, une sphère ou une capsule devient lisse).
		constexpr float MaxSmoothingAngle = 80.0f; // en degrés
		importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS);
		importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, MaxSmoothingAngle);

		// Pas de aiProcess_ImproveCacheLocality : très lent sur des millions de triangles
		// pour un gain quasi nul sur les GPU actuels.
		const aiScene* scene = importer.ReadFile(_path,
			aiProcess_RemoveComponent |        // supprime normales / tangentes du fichier (voir AI_CONFIG_PP_RVC_FLAGS)
			aiProcess_Triangulate |            // tout en triangles
			aiProcess_GenSmoothNormals |       // recalcule des normales lissées
			aiProcess_CalcTangentSpace |       // tangentes pour le normal mapping (à partir des UV)
			aiProcess_JoinIdenticalVertices |  // fusionne les sommets identiques (beaucoup moins de VRAM)
			aiProcess_PreTransformVertices);   // applique la hiérarchie de nodes FBX (regroupe les sous-meshes par matériau)

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->HasMeshes())
		{
			std::cout << "Mesh impossible à charger '" << _path << "' : " << importer.GetErrorString() << std::endl;
			return false;
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

		// ---------- Slots : un par matériau du fichier réellement utilisé ----------
		// slotOfMaterial[index du matériau Assimp] = index du slot (-1 = pas encore vu)
		std::vector<int> slotOfMaterial(glm::max(scene->mNumMaterials, 1u), -1);
		std::vector<std::vector<uint32_t>> meshesOfSlot;
		_data.SlotNames.clear();

		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			uint32_t materialIndex = scene->mMeshes[m]->mMaterialIndex;
			if (materialIndex >= slotOfMaterial.size())
				materialIndex = 0;

			if (slotOfMaterial[materialIndex] < 0)
			{
				slotOfMaterial[materialIndex] = (int)_data.SlotNames.size();

				std::string name;
				if (materialIndex < scene->mNumMaterials)
					name = scene->mMaterials[materialIndex]->GetName().C_Str();
				if (name.empty())
					name = "Material " + std::to_string(_data.SlotNames.size());

				_data.SlotNames.push_back(name);
				meshesOfSlot.emplace_back();
			}
			meshesOfSlot[slotOfMaterial[materialIndex]].push_back(m);
		}

		// Réserve tout d'un coup (sinon des dizaines de réallocations sur un gros modèle)
		size_t vertexCount = 0, indexCount = 0;
		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			vertexCount += scene->mMeshes[m]->mNumVertices;
			indexCount += (size_t)scene->mMeshes[m]->mNumFaces * 3;
		}
		_data.Vertices.clear();
		_data.Indices.clear();
		_data.SubMeshes.clear();
		_data.Vertices.reserve(vertexCount);
		_data.Indices.reserve(indexCount);

		// ---------- Sommets : tous les sous-meshes dans un seul vertex buffer ----------
		std::vector<uint32_t> baseVertexOfMesh(scene->mNumMeshes, 0);
		for (uint32_t m = 0; m < scene->mNumMeshes; m++)
		{
			const aiMesh* mesh = scene->mMeshes[m];
			baseVertexOfMesh[m] = (uint32_t)_data.Vertices.size();
			const bool hasNormals = mesh->HasNormals();
			const bool hasUVs = mesh->HasTextureCoords(0);
			const bool hasTangents = mesh->HasTangentsAndBitangents();

			for (uint32_t v = 0; v < mesh->mNumVertices; v++)
			{
				MeshVertex vertex;
				vertex.Position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z) * scale;
				vertex.Normal = hasNormals
					? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
					: glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.TexCoord = hasUVs
					? glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
					: glm::vec2(0.0f);

				if (hasTangents)
				{
					glm::vec3 t(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
					glm::vec3 b(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
					// UV en miroir : la bitangente pointe dans l'autre sens que cross(N, T)
					float sign = glm::dot(glm::cross(vertex.Normal, t), b) < 0.0f ? -1.0f : 1.0f;
					vertex.Tangent = glm::vec4(t, sign);
				}
				_data.Vertices.push_back(vertex);
			}
		}

		// ---------- Indices regroupés par slot : un seul draw call par matériau ----------
		for (uint32_t slot = 0; slot < (uint32_t)meshesOfSlot.size(); slot++)
		{
			SubMesh sub;
			sub.BaseIndex = (uint32_t)_data.Indices.size();
			sub.MaterialIndex = slot;

			for (uint32_t m : meshesOfSlot[slot])
			{
				const aiMesh* mesh = scene->mMeshes[m];
				const uint32_t baseVertex = baseVertexOfMesh[m];

				for (uint32_t f = 0; f < mesh->mNumFaces; f++)
				{
					const aiFace& face = mesh->mFaces[f];
					if (face.mNumIndices != 3) // ignore points / lignes
						continue;

					_data.Indices.push_back(baseVertex + face.mIndices[0]);
					_data.Indices.push_back(baseVertex + face.mIndices[1]);
					_data.Indices.push_back(baseVertex + face.mIndices[2]);
				}
			}

			sub.IndexCount = (uint32_t)_data.Indices.size() - sub.BaseIndex;
			if (sub.IndexCount > 0)
				_data.SubMeshes.push_back(sub);
		}
		return true;
	}

	static std::unordered_map<std::string, Ref<Mesh>> s_Cache;

	static std::string MeshCacheKey(const std::string& _path)
	{
		const AssetID id = AssetsManager::GetID(_path);
		return id != 0 ? "#" + std::to_string(id) : _path;
	}

	Ref<Mesh> Mesh::GetLoaded(const std::string& _rawPath)
	{
		auto it = s_Cache.find(MeshCacheKey(AssetsManager::Resolve(_rawPath)));
		return it != s_Cache.end() ? it->second : nullptr;
	}

	Ref<Mesh> Mesh::Create(const std::string& _rawPath)
	{

		// Chemin actuel (le fichier a pu être renommé / déplacé) et clé = identifiant de l'asset
		const std::string _path = AssetsManager::Resolve(_rawPath);
		const AssetID id = AssetsManager::GetID(_path);
		const std::string key = id != 0 ? "#" + std::to_string(id) : _path;

		auto it = s_Cache.find(key);
		if (it != s_Cache.end())
			return it->second;

		const auto start = std::chrono::steady_clock::now();

		MeshData data;
		bool fromCache = false;

		if (id != 0 && ReadCooked(GetCookedPath(id), _path, data))
		{
			fromCache = true;
		}
		else
		{
			data = MeshData();
			if (!ImportWithAssimp(_path, data))
				return nullptr;

			if (id != 0)
				WriteCooked(GetCookedPath(id), data);
		}

		const std::vector<uint32_t>& indices = data.Indices;
		const std::vector<MeshVertex>& vertices = data.Vertices;
		Ref<Mesh> result(new Mesh(vertices, indices, data.SubMeshes, data.SlotNames));
		result->m_Path = _path;
		s_Cache[key] = result;

		const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
		std::cout << "Mesh '" << _path << "' : " << indices.size() / 3 << " triangles, " << vertices.size() << " sommets, " << result->GetMaterialSlotCount() << " materiau(x).. charge en " << seconds << std::endl;

		return result;
	}

	Mesh::Mesh(const std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices,
		const std::vector<SubMesh>& _subMeshes, const std::vector<std::string>& _materialSlotNames)
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
			{ ShaderDataType::Float4, "a_Tangent" },
		});
		m_VertexArray->AddVertexBuffer(vb);

		Ref<IndexBuffer> ib;
		ib.reset(IndexBuffer::Create(const_cast<uint32_t*>(_indices.data()), (uint32_t)_indices.size()));
		m_VertexArray->SetIndexBuffer(ib);

		m_VertexArray->Unbind(); // ne laisse aucun VAO bindé pour le prochain code qui crée des buffers

		m_IndexCount = (uint32_t)_indices.size();

		// Boîte englobante (frustum culling)
		if (!_vertices.empty())
		{
			m_Bounds.Min = m_Bounds.Max = _vertices[0].Position;
			for (const MeshVertex& v : _vertices)
			{
				m_Bounds.Min = glm::min(m_Bounds.Min, v.Position);
				m_Bounds.Max = glm::max(m_Bounds.Max, v.Position);
			}
		}

		// ---------- Sous-meshes / slots de matériaux ----------
		m_MaterialSlotNames = _materialSlotNames;
		m_SubMeshes = _subMeshes;
		if (m_SubMeshes.empty())
		{
			SubMesh all;
			all.IndexCount = m_IndexCount;
			m_SubMeshes.push_back(all);
		}

		uint32_t slotCount = 0;
		for (const SubMesh& sub : m_SubMeshes)
			slotCount = glm::max(slotCount, sub.MaterialIndex + 1);
		while (m_MaterialSlotNames.size() < slotCount)
			m_MaterialSlotNames.push_back(m_MaterialSlotNames.empty() ? std::string("Material") : "Material " + std::to_string(m_MaterialSlotNames.size()));

		// Boîte englobante de chaque sous-mesh
		for (SubMesh& sub : m_SubMeshes)
		{
			sub.Bounds = m_Bounds;
			if (sub.IndexCount == 0)
				continue;

			sub.Bounds.Min = sub.Bounds.Max = _vertices[_indices[sub.BaseIndex]].Position;
			for (uint32_t i = sub.BaseIndex; i < sub.BaseIndex + sub.IndexCount; i++)
			{
				const glm::vec3& p = _vertices[_indices[i]].Position;
				sub.Bounds.Min = glm::min(sub.Bounds.Min, p);
				sub.Bounds.Max = glm::max(sub.Bounds.Max, p);
			}
		}
	}

	// Tangentes par triangle (méthode de Lengyel), moyennées sur les sommets partagés puis orthogonalisées
	void Mesh::ComputeTangents(std::vector<MeshVertex>& _vertices, const std::vector<uint32_t>& _indices)
	{
		std::vector<glm::vec3> tangents(_vertices.size(), glm::vec3(0.0f));
		std::vector<glm::vec3> bitangents(_vertices.size(), glm::vec3(0.0f));

		for (size_t i = 0; i + 2 < _indices.size(); i += 3)
		{
			const uint32_t i0 = _indices[i], i1 = _indices[i + 1], i2 = _indices[i + 2];
			const MeshVertex& v0 = _vertices[i0];
			const MeshVertex& v1 = _vertices[i1];
			const MeshVertex& v2 = _vertices[i2];

			const glm::vec3 e1 = v1.Position - v0.Position;
			const glm::vec3 e2 = v2.Position - v0.Position;
			const glm::vec2 d1 = v1.TexCoord - v0.TexCoord;
			const glm::vec2 d2 = v2.TexCoord - v0.TexCoord;

			const float det = d1.x * d2.y - d2.x * d1.y;
			if (std::abs(det) < 1e-12f)
				continue; // UV dégénérées

			const float r = 1.0f / det;
			const glm::vec3 t = (e1 * d2.y - e2 * d1.y) * r;
			const glm::vec3 b = (e2 * d1.x - e1 * d2.x) * r;

			for (uint32_t idx : { i0, i1, i2 })
			{
				tangents[idx] += t;
				bitangents[idx] += b;
			}
		}

		for (size_t v = 0; v < _vertices.size(); v++)
		{
			const glm::vec3& n = _vertices[v].Normal;
			glm::vec3 t = tangents[v] - n * glm::dot(n, tangents[v]); // Gram-Schmidt
			if (glm::dot(t, t) < 1e-12f)
			{
				_vertices[v].Tangent = glm::vec4(0.0f);
				continue;
			}
			t = glm::normalize(t);
			const float sign = glm::dot(glm::cross(n, t), bitangents[v]) < 0.0f ? -1.0f : 1.0f;
			_vertices[v].Tangent = glm::vec4(t, sign);
		}
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
		std::vector<uint32_t> indices = MakeQuadIndices(1);
		ComputeTangents(v, indices);
		return Ref<Mesh>(new Mesh(v, indices));
	}

	Ref<Mesh> Mesh::CreatePlane()
	{
		std::vector<MeshVertex> v = {
			{ { -0.5f, 0.0f,  0.5f }, { 0, 1, 0 }, { 0, 0 } },
			{ {  0.5f, 0.0f,  0.5f }, { 0, 1, 0 }, { 1, 0 } },
			{ {  0.5f, 0.0f, -0.5f }, { 0, 1, 0 }, { 1, 1 } },
			{ { -0.5f, 0.0f, -0.5f }, { 0, 1, 0 }, { 0, 1 } },
		};
		std::vector<uint32_t> indices = MakeQuadIndices(1);
		ComputeTangents(v, indices);
		return Ref<Mesh>(new Mesh(v, indices));
	}

	Ref<Mesh> Mesh::CreateSphere(uint32_t _segments, uint32_t _rings)
	{
		_segments = glm::max(_segments, 3u);
		_rings = glm::max(_rings, 2u);
		constexpr float pi = 3.14159265358979f;

		std::vector<MeshVertex> vertices;
		vertices.reserve((size_t)(_segments + 1) * (_rings + 1));
		for (uint32_t r = 0; r <= _rings; r++)
		{
			const float theta = (float)r / (float)_rings * pi; // 0 = pôle nord (+Y)
			for (uint32_t s = 0; s <= _segments; s++)
			{
				const float phi = (float)s / (float)_segments * 2.0f * pi;
				const glm::vec3 n(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));

				MeshVertex v;
				v.Position = n * 0.5f;
				v.Normal = n;
				v.TexCoord = { (float)s / (float)_segments, 1.0f - (float)r / (float)_rings };
				vertices.push_back(v);
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve((size_t)_segments * _rings * 6);
		for (uint32_t r = 0; r < _rings; r++)
		{
			for (uint32_t s = 0; s < _segments; s++)
			{
				const uint32_t a = r * (_segments + 1) + s;
				const uint32_t b = a + _segments + 1;
				indices.insert(indices.end(), { a, a + 1, b, a + 1, b + 1, b }); // sens anti-horaire vu de l'extérieur
			}
		}

		ComputeTangents(vertices, indices);
		return Ref<Mesh>(new Mesh(vertices, indices));
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
		std::vector<uint32_t> indices = MakeQuadIndices(6);
		ComputeTangents(v, indices);
		return Ref<Mesh>(new Mesh(v, indices));
	}
}
