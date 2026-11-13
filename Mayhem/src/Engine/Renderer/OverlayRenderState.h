#pragma once

namespace Mayhem
{
	// État OpenGL pour les éléments d'éditeur dessinés par-dessus la scène (grille…) :
	// transparence activée, test de profondeur actif mais sans écrire la profondeur,
	// et l'attachement des entity IDs protégé (la grille ne bloque pas la sélection à la souris).
	class OverlayRenderState
	{
	public:
		void Begin();
		void End();

	private:
		bool m_BlendWasEnabled = false;
		bool m_DepthTestWasEnabled = false;
		unsigned char m_DepthMaskWas = 1;
	};
}
