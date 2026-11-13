#pragma once

#include <cstdint>

namespace Mayhem
{
	// Shadow map en cascades : une texture de profondeur "tableau", une couche par cascade.
	// Implémentation OpenGL (à déplacer dans Platform/OpenGL si tu sépares les backends).
	class ShadowMap
	{
	public:
		ShadowMap(uint32_t _resolution = 2048, uint32_t _layerCount = 4);
		~ShadowMap();

		ShadowMap(const ShadowMap&) = delete;
		ShadowMap& operator=(const ShadowMap&) = delete;

		// Sauvegarde le framebuffer/viewport courants et binde la shadow map
		void BeginPass();
		// Rend dans la couche _layer (et vide sa profondeur)
		void BindLayer(uint32_t _layer);
		// Restaure le framebuffer/viewport d'avant (ex : le framebuffer du viewport de l'éditeur)
		void EndPass();

		void BindTexture(uint32_t _slot) const;

		void Resize(uint32_t _resolution);
		uint32_t GetResolution() const { return m_Resolution; }
		uint32_t GetLayerCount() const { return m_LayerCount; }

	private:
		void Invalidate();
		void Release();

	private:
		uint32_t m_FBO = 0;
		uint32_t m_DepthTexture = 0;
		uint32_t m_Resolution = 0;
		uint32_t m_LayerCount = 0;

		int m_PrevFBO = 0;
		int m_PrevViewport[4] = { 0, 0, 0, 0 };
	};
}
