#include "EditorLayer.h"
#include "Engine/Core/EntryPoint.h"
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Mayhem
{

	class MayhemEditor : public Mayhem::Application
	{
	public:
		MayhemEditor(std::string _name) : Mayhem::Application(_name)
		{
			PushLayer(new EditorLayer());
		}

		~MayhemEditor()
		{

		}
	};

	Application* Mayhem::CreateApplication()
	{
		return new MayhemEditor("Mayhem Editor");
	}
}