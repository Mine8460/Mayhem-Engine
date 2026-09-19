#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Events/Event.h>
#include <Engine/Core/Timestep.h>

namespace Mayhem
{
	class ENGINE_API Layer
	{
	public:
		Layer(const std::string& _name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep _timestep) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;

	};
}
