#include "Sandbox2D.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>

#define PROFILE_FUNCTION(name) Timer timer##__LINE__(name, [&](ProfileResult _result) {m_ProfileResults.push_back(_result); })

template<typename Fn>
class Timer
{
public:
	Timer(const char* _name, Fn&& _func) : m_Name(_name), m_Stopped(false), m_Func(_func)
	{
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}
	~Timer(void)
	{
		if (!m_Stopped)
			Stop();
	}

	void Stop(void)
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		m_Stopped = true;

		float duration = (end - start) * 0.001f;
		m_Func({ m_Name, duration });
	}

private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
	bool m_Stopped = false;
	Fn m_Func;
};


EditorLayer::EditorLayer() : Layer("Sandbox2D")
{
	m_CameraController = std::make_shared<Mayhem::OrthographicCameraController>(1280.0f / 720.0f, true);
}

void EditorLayer::OnAttach()
{
	m_Texture = Mayhem::Texture2D::Create("assets/textures/Checkerboard.png");
	m_AlphaTexture = Mayhem::Texture2D::Create("assets/textures/AlphaCheckerboard.png");
	m_SpriteSheet = Mayhem::Texture2D::Create("assets/game/Spritesheet.png");
	m_Sprites = Mayhem::SubTexture2D::CreateAllSpriteSheet(m_SpriteSheet, { 64.f,64.f });

    Mayhem::FrameBufferSpecification spec;
    spec.Width = 1280;
    spec.Height = 720;
}

void EditorLayer::OnDetach()
{
}

void EditorLayer::OnUpdate(Mayhem::Timestep _timestep)
{
	PROFILE_FUNCTION("Sandbox2D::OnUpdate");
	// Update
	m_CameraController->OnUpdate(_timestep);

	// Statistics
	Mayhem::Renderer2D::ResetStats();
	// Render
	{
		static float rotation = 0.0f;
		rotation += _timestep.GetSeconds() * 50.0f;



		PROFILE_FUNCTION("Sandbox2D::OnRender");
		Mayhem::RenderCommand::Clear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		Mayhem::Renderer2D::BeginScene(m_CameraController->GetCamera());

		Mayhem::Renderer2D::DrawQuad({ 0.f,0.f }, { 0.9f, 0.9f }, m_Sprites[spriteToUse], glm::vec4(1.0f), 1.f);
		Mayhem::Renderer2D::DrawQuad({ 1.f,0.f }, { 0.9f, 0.9f }, m_Texture, glm::vec4(1.0f), 1.f);

		Mayhem::Renderer2D::EndScene();
	}
}

void EditorLayer::OnImGuiRender()
{
    ImGui::Begin("Settings");

    ImGui::DragInt("Sprite", &spriteToUse, 1.0, 1, m_Sprites.size() - 2);

    ImGui::End();
    ImGui::Begin("Profiling");

    Mayhem::Renderer2D::Statistics stats = Mayhem::Renderer2D::GetStats();

    ImGui::Text("Renderer2D Stats:");
    ImGui::Text("Draw Calls: %d", stats.DrawCalls);
    ImGui::Text("Quads: %d", stats.QuadCount);
    ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
    ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
    ImGui::Text("");

    for (ProfileResult& result : m_ProfileResults)
    {
        char label[75];
        strcpy(label, "  %.3fms ");
        strcat(label, result.Name);
        ImGui::Text(label, result.Time);
    }

    m_ProfileResults.clear();

    ImGui::End();
}

void EditorLayer::OnEvent(Mayhem::Event& _e)
{
	m_CameraController->OnEvent(_e);
}
