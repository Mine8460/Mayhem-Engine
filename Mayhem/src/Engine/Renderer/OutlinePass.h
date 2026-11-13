#pragma once

#include <cstdint>

namespace Mayhem
{
	// Contour de sélection de l'éditeur : masque des objets sélectionnés + composition du contour.
	// Implémentation OpenGL (à déplacer dans Platform/OpenGL si tu sépares les backends).
	class OutlinePass
	{
	public:
		OutlinePass() = default;
		~OutlinePass();

		OutlinePass(const OutlinePass&) = delete;
		OutlinePass& operator=(const OutlinePass&) = delete;

		// Rend dans le masque (taille = viewport courant). Renvoie false si le viewport est vide.
		bool BeginMask();
		void EndMask();

		// Dessine par-dessus le framebuffer courant, sans toucher à l'attachement des entity IDs
		void BeginComposite();
		void EndComposite();

		void BindMaskTexture(uint32_t _slot) const;

	private:
		void Resize(uint32_t _width, uint32_t _height);
		void Release();

	private:
		uint32_t m_FBO = 0;
		uint32_t m_MaskTexture = 0;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		int m_PrevFBO = 0;
		int m_PrevViewport[4] = { 0, 0, 0, 0 };

		bool m_DepthTestWasEnabled = false;
		bool m_BlendWasEnabled = false;
	};
}
