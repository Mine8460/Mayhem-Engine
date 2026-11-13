#include "ThumbnailRenderer.h"

#include <Engine/Renderer/SceneRenderer.h>
#include <Engine/Renderer/RenderCommand.h>
#include <Engine/Renderer/Camera/Camera.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Mayhem
{
	// =====================================================================
	//  ThumbnailTexture
	// =====================================================================

	ThumbnailTexture::ThumbnailTexture(const Ref<FrameBuffer>& _frameBuffer)
		: m_FrameBuffer(_frameBuffer)
	{
	}

	uint32_t ThumbnailTexture::GetWidth() const { return m_FrameBuffer->GetSpecification().Width; }
	uint32_t ThumbnailTexture::GetHeight() const { return m_FrameBuffer->GetSpecification().Height; }
	uint32_t ThumbnailTexture::GetRendererID() const { return m_FrameBuffer->GetColorAttachmentRendererID(0); }

	void ThumbnailTexture::Bind(uint32_t) const
	{
		// Une miniature n'est jamais utilisée comme texture de rendu : seulement affichée par ImGui (GetRendererID)
		MAYHEM_CORE_ASSERT(false, "ThumbnailTexture::Bind n'est pas supporte");
	}

	// =====================================================================
	//  Rendu
	// =====================================================================

	namespace
	{
		constexpr float s_FieldOfView = glm::radians(30.0f);
		constexpr float s_Fill = 0.9f; // part du cadre occupée par l'objet

		Ref<Mesh> s_Sphere;
		Ref<Material> s_NeutralMaterial;

		Ref<ThumbnailTexture> PrepareTarget(const Ref<ThumbnailTexture>& _reuse)
		{
			if (_reuse)
				return _reuse;

			FrameBufferSpecification spec;
			spec.Attachment = { FrameBufferTextureFormat::RGBA8, FrameBufferTextureFormat::Depth };
			spec.Width = ThumbnailRenderer::Size;
			spec.Height = ThumbnailRenderer::Size;
			return Ref<ThumbnailTexture>(new ThumbnailTexture(FrameBuffer::Create(spec)));
		}

		// Caméra regardant _target depuis la direction _fromDirection (normalisée), à la distance _distance
		struct ThumbnailCamera
		{
			glm::mat4 View;
			glm::mat4 Projection;
		};

		ThumbnailCamera MakeCamera(const glm::vec3& _target, const glm::vec3& _fromDirection, float _distance, float _radius)
		{
			ThumbnailCamera camera;
			const glm::vec3 eye = _target + _fromDirection * _distance;
			camera.View = glm::lookAt(eye, _target, glm::vec3(0.0f, 1.0f, 0.0f));
			camera.Projection = glm::perspective(s_FieldOfView, 1.0f,
				glm::max(_distance - _radius * 2.0f, _distance * 0.02f), _distance + _radius * 2.0f);
			return camera;
		}

		// Éclairage de studio, défini dans le repère de la caméra : lumière venant du haut-gauche, devant l'objet
		void DrawLit(const Ref<ThumbnailTexture>& _target, const ThumbnailCamera& _camera, const Ref<Mesh>& _mesh, const Ref<Material>& _material)
		{
			const Ref<FrameBuffer>& frameBuffer = _target->GetFrameBuffer();
			frameBuffer->Bind();
			RenderCommand::Clear(glm::vec4(0.20f, 0.21f, 0.23f, 1.0f));

			const glm::mat4 cameraTransform = glm::inverse(_camera.View);
			SceneRenderer::BeginScene(Camera(_camera.Projection), cameraTransform);

			const glm::vec3 lightDirection = glm::mat3(cameraTransform) * glm::normalize(glm::vec3(0.45f, -0.55f, -0.70f));
			SceneRenderer::SetDirectionalLightDirection(lightDirection, glm::vec3(1.0f), 1.1f, 0.45f);
			SceneRenderer::DisableShadows();
			// Environnement de studio : haut clair, horizon neutre, sol sombre (les reflets des métaux en dépendent)
			SceneRenderer::SetEnvironmentGradient(glm::vec3(0.85f), glm::vec3(0.62f), glm::vec3(0.22f), 1.0f);

			SceneRenderer::DrawMesh(_mesh, glm::mat4(1.0f), _material);

			// Pas de grille, de contour ni d'arêtes de l'éditeur dans une miniature
			SceneRenderer::SuspendOverlays(true);
			SceneRenderer::EndScene();
			SceneRenderer::SuspendOverlays(false);

			frameBuffer->Unbind();
		}
	}

	Ref<ThumbnailTexture> ThumbnailRenderer::RenderMaterial(const Ref<Material>& _material, const Ref<ThumbnailTexture>& _reuse)
	{
		if (!_material)
			return nullptr;

		if (!s_Sphere)
			s_Sphere = Mesh::CreateSphere();

		Ref<ThumbnailTexture> target = PrepareTarget(_reuse);

		// Sphère de rayon 0.5 : la distance qui la fait remplir s_Fill du demi-champ de vision
		const float radius = 0.5f;
		const float distance = radius / std::sin(s_FieldOfView * 0.5f * s_Fill);
		ThumbnailCamera camera = MakeCamera(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), distance, radius);

		DrawLit(target, camera, s_Sphere, _material);
		return target;
	}

	Ref<ThumbnailTexture> ThumbnailRenderer::RenderMesh(const Ref<Mesh>& _mesh, const Ref<ThumbnailTexture>& _reuse)
	{
		if (!_mesh)
			return nullptr;

		if (!s_NeutralMaterial)
		{
			s_NeutralMaterial = Ref<Material>(new Material());
			s_NeutralMaterial->m_AlbedoColor = glm::vec4(0.78f, 0.78f, 0.80f, 1.0f);
			s_NeutralMaterial->m_Roughness = 0.55f;
		}

		Ref<ThumbnailTexture> target = PrepareTarget(_reuse);

		const AABB& bounds = _mesh->GetBounds();
		const glm::vec3 center = bounds.GetCenter();
		const float radius = glm::max(glm::length(bounds.Max - bounds.Min) * 0.5f, 1e-4f);

		// Vue 3/4 : de face, légèrement à droite et au-dessus
		const glm::vec3 direction = glm::normalize(glm::vec3(0.55f, 0.40f, 1.0f));
		const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), direction));
		const glm::vec3 up = glm::cross(direction, right);

		// Distance minimale pour que les 8 coins de la boîte restent dans le cadre
		const float tanHalf = std::tan(s_FieldOfView * 0.5f) * s_Fill;
		float distance = radius;
		for (int i = 0; i < 8; i++)
		{
			const glm::vec3 corner((i & 1) ? bounds.Max.x : bounds.Min.x,
			                       (i & 2) ? bounds.Max.y : bounds.Min.y,
			                       (i & 4) ? bounds.Max.z : bounds.Min.z);
			const glm::vec3 p = corner - center;
			const float along = glm::dot(p, direction);
			distance = glm::max(distance, glm::abs(glm::dot(p, right)) / tanHalf + along);
			distance = glm::max(distance, glm::abs(glm::dot(p, up)) / tanHalf + along);
		}

		ThumbnailCamera camera = MakeCamera(center, direction, distance, radius);
		DrawLit(target, camera, _mesh, s_NeutralMaterial);
		return target;
	}

	void ThumbnailRenderer::Shutdown()
	{
		s_Sphere.reset();
		s_NeutralMaterial.reset();
	}
}
