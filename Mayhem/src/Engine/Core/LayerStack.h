#pragma once

#include <Engine/Core/Core.h>
#include "Layer.h"

#include <vector>

namespace Mayhem
{
	class ENGINE_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* _layer);
		void PushOverlay(Layer* _overlay);
		void PopLayer(Layer* _layer);
		void PopOverlay(Layer* _overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); };
		std::vector<Layer*>::iterator end() { return m_Layers.end(); };

	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayersInsertIndex = 0;
	};
}
