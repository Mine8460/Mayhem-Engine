#include "Enpch.h"
#include "LayerStack.h"

namespace Mayhem
{
	LayerStack::LayerStack()
	{
	}
	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
		{
			delete layer;
		}
	}
	void LayerStack::PushLayer(Layer* _layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayersInsertIndex, _layer);
		m_LayersInsertIndex++;
		_layer->OnAttach();
	}

	void LayerStack::PushOverlay(Layer* _overlay)
	{
		m_Layers.emplace_back(_overlay);
		_overlay->OnAttach();
	}

	void LayerStack::PopLayer(Layer* _layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), _layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
			m_LayersInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* _overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), _overlay);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
		}
	}

}
