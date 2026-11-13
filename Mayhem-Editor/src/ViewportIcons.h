#pragma once

#include <Engine.h>
#include <Engine/Scene/Scene.h>

#include <imgui/imgui.h>
#include <glm/glm.hpp>

namespace Mayhem
{
	// Icônes de l'éditeur dans le viewport pour les objets sans rendu (caméra, lumière, ciel),
	// + frustum de la caméra sélectionnée et direction de la lumière sélectionnée.
	// Dessiné avec ImGui par-dessus l'image de la scène : à appeler dans la fenêtre "Viewport".
	class ViewportIcons
	{
	public:
		// Renvoie l'entité dont l'icône est sous la souris (entité nulle sinon)
		Entity Draw(const Ref<Scene>& _scene, const glm::mat4& _viewProjection,
			const ImVec2& _viewportMin, const ImVec2& _viewportMax, Entity _selected);

		// Couleur de l'icône sélectionnée (reprends celle de ton contour de sélection)
		ImU32 SelectedColor = IM_COL32(179, 89, 255, 255);
	};
}
