#include "Sandbox2D.h"
#include "Engine/Core/EntryPoint.h"
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Sandbox : public Mayhem::Application
{
public:
	Sandbox(std::string _name) : Mayhem::Application(_name)
	{
		PushLayer(new EditorLayer());
	}

	~Sandbox()
	{
		
	}
};

Mayhem::Application* Mayhem::CreateApplication()
{
	return new Sandbox("Sandbox");
}