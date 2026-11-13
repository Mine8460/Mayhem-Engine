#pragma once

#include <Engine.h>
#include <Engine/Renderer/Material.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/Renderer/FrameBuffer.h>
#include <Engine/Renderer/Texture.h>

namespace Mayhem
{
	// Texture dont les pixels viennent d'un FrameBuffer (miniature rendue par le moteur).
	// Utilisable partout où on affiche un Texture2D avec GetRendererID() dans ImGui.
	class ThumbnailTexture : public Texture2D
	{
	public:
		explicit ThumbnailTexture(const Ref<FrameBuffer>& _frameBuffer);

		virtual uint32_t GetWidth() const override;
		virtual uint32_t GetHeight() const override;
		virtual uint32_t GetRendererID() const override;
		virtual void SetData(void*, uint32_t) override {}
		virtual void Bind(uint32_t _slot = 0) const override;
		virtual bool operator==(const Texture& _other) const override { return GetRendererID() == _other.GetRendererID(); }
		virtual std::string GetPath() override { return {}; }

		const Ref<FrameBuffer>& GetFrameBuffer() const { return m_FrameBuffer; }

	private:
		Ref<FrameBuffer> m_FrameBuffer;
	};

	// Génère les miniatures 3D du Content Browser : un matériau sur une sphère, un modèle vu en perspective.
	// À appeler pendant le rendu ImGui (jamais entre SceneRenderer::BeginScene et EndScene).
	class ThumbnailRenderer
	{
	public:
		static constexpr uint32_t Size = 192; // pixels, affichée plus petit dans l'interface = lissage gratuit

		// Matériau appliqué sur une sphère. _reuse : miniature à redessiner (sinon une nouvelle est créée).
		static Ref<ThumbnailTexture> RenderMaterial(const Ref<Material>& _material, const Ref<ThumbnailTexture>& _reuse = nullptr);

		// Modèle 3D cadré sur sa boîte englobante, avec un matériau neutre
		static Ref<ThumbnailTexture> RenderMesh(const Ref<Mesh>& _mesh, const Ref<ThumbnailTexture>& _reuse = nullptr);

		// Libère les ressources partagées (sphère, matériau neutre)
		static void Shutdown();
	};
}
