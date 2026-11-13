#include "EditorLayer.h"
#include "ThumbnailRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <entt.hpp>
#include <Engine/Scene/SceneSerializer.h>

#include <Engine/Utils/PlatformUtils.h>
#include <Engine/Core/MouseCode.h>
#include <Engine/Core/AssetsManager.h>
#include <Engine/Scene/Prefab.h>

#define IM_PI 3.14159265358979323846f
// =====================================================================
//  Boutons d'outils dessinés à la main (aucune texture à charger)
// =====================================================================

// Entité sélectionnée + tous ses enfants (comme Unity : sélectionner un parent entoure tout l'objet)
static void CollectOutlinedEntities(Mayhem::Entity _entity, std::vector<int>& _out)
{
	_out.push_back((int)(uint32_t)_entity);
	for (Mayhem::Entity child : _entity.GetChildren())
		CollectOutlinedEntities(child, _out);
}

using IconDrawer = void(*)(ImDrawList*, ImVec2 /*centre*/, float /*demi-taille*/, ImU32 /*couleur*/);

static bool ToolButton(const char* _id, bool _active, float _size, IconDrawer _drawIcon, const char* _tooltip = nullptr)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	bool clicked = ImGui::InvisibleButton(_id, ImVec2(_size, _size));
	bool hovered = ImGui::IsItemHovered();
	bool held = ImGui::IsItemActive();

	ImU32 bg;
	if (_active)       bg = IM_COL32(140, 70, 210, 255); // bleu "actif" façon Unity
	else if (held)     bg = IM_COL32(80, 80, 86, 255);
	else if (hovered)  bg = IM_COL32(66, 66, 72, 255);
	else               bg = IM_COL32(48, 48, 53, 255);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(pos, ImVec2(pos.x + _size, pos.y + _size), bg, 4.0f);
	_drawIcon(dl, ImVec2(pos.x + _size * 0.5f, pos.y + _size * 0.5f), _size * 0.5f, IM_COL32(225, 225, 225, 255));

	if (_tooltip && hovered)
		ImGui::SetTooltip("%s", _tooltip);

	return clicked;
}

// Local : cube penché avec ses propres axes
static void IconLocal(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.42f, o = r * 0.18f;
	ImVec2 f0(c.x - s, c.y - s + o), f1(c.x + s - o, c.y - s + o), f2(c.x + s - o, c.y + s), f3(c.x - s, c.y + s);
	ImVec2 b0(f0.x + o, f0.y - o), b1(f1.x + o, f1.y - o), b2(f2.x + o, f2.y - o);
	dl->AddQuad(f0, f1, f2, f3, col, 1.4f);
	dl->AddLine(f0, b0, col, 1.4f);
	dl->AddLine(f1, b1, col, 1.4f);
	dl->AddLine(f2, b2, col, 1.4f);
	dl->AddLine(b0, b1, col, 1.4f);
	dl->AddLine(b1, b2, col, 1.4f);
}

// World : globe (cercle + équateur + méridien)
static void IconWorld(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.55f;
	dl->AddCircle(c, s, col, 20, 1.4f);
	dl->AddLine(ImVec2(c.x - s, c.y), ImVec2(c.x + s, c.y), col, 1.2f);
	dl->AddBezierCubic(ImVec2(c.x, c.y - s), ImVec2(c.x + s * 0.55f, c.y - s * 0.5f),
		ImVec2(c.x + s * 0.55f, c.y + s * 0.5f), ImVec2(c.x, c.y + s), col, 1.2f);
	dl->AddBezierCubic(ImVec2(c.x, c.y - s), ImVec2(c.x - s * 0.55f, c.y - s * 0.5f),
		ImVec2(c.x - s * 0.55f, c.y + s * 0.5f), ImVec2(c.x, c.y + s), col, 1.2f);
}

static void IconPlay(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	dl->AddTriangleFilled(ImVec2(c.x - r * 0.35f, c.y - r * 0.45f), ImVec2(c.x + r * 0.45f, c.y), ImVec2(c.x - r * 0.35f, c.y + r * 0.45f), col);
}

static void IconStop(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	dl->AddRectFilled(ImVec2(c.x - r * 0.38f, c.y - r * 0.38f), ImVec2(c.x + r * 0.38f, c.y + r * 0.38f), col, 2.0f);
}

static void IconGrid(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.55f;
	for (int i = 0; i < 3; i++)
	{
		float o = -s + i * s; // 3 lignes de chaque côté : -s, 0, +s
		dl->AddLine(ImVec2(c.x - s, c.y + o), ImVec2(c.x + s, c.y + o), col, 1.2f);
		dl->AddLine(ImVec2(c.x + o, c.y - s), ImVec2(c.x + o, c.y + s), col, 1.2f);
	}
}

// Wireframe : cube en fil de fer (hexagone + arêtes vues de face)
static void IconWireframe(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	const float s = r * 0.58f;
	const float h = s * 0.5f;
	ImVec2 hex[] = {
		{ c.x, c.y - s }, { c.x + s * 0.87f, c.y - h }, { c.x + s * 0.87f, c.y + h },
		{ c.x, c.y + s }, { c.x - s * 0.87f, c.y + h }, { c.x - s * 0.87f, c.y - h }
	};
	dl->AddPolyline(hex, IM_ARRAYSIZE(hex), col, ImDrawFlags_Closed, 1.3f);
	dl->AddLine(c, hex[1], col, 1.2f);
	dl->AddLine(c, hex[5], col, 1.2f);
	dl->AddLine(c, hex[3], col, 1.2f);
}

static void IconSelect(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.55f;
	ImVec2 pts[] = {
		{ c.x - 0.60f * s, c.y - 0.90f * s }, { c.x - 0.60f * s, c.y + 0.60f * s }, { c.x - 0.20f * s, c.y + 0.25f * s },
		{ c.x + 0.10f * s, c.y + 0.90f * s }, { c.x + 0.35f * s, c.y + 0.78f * s }, { c.x + 0.05f * s, c.y + 0.15f * s },
		{ c.x + 0.55f * s, c.y + 0.15f * s }
	};
	dl->AddPolyline(pts, IM_ARRAYSIZE(pts), col, ImDrawFlags_Closed, 1.5f);
}

static void IconMove(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.6f, h = r * 0.18f, l = h * 1.6f;
	dl->AddLine(ImVec2(c.x - s + l, c.y), ImVec2(c.x + s - l, c.y), col, 1.5f);
	dl->AddLine(ImVec2(c.x, c.y - s + l), ImVec2(c.x, c.y + s - l), col, 1.5f);
	dl->AddTriangleFilled(ImVec2(c.x + s, c.y), ImVec2(c.x + s - l, c.y + h), ImVec2(c.x + s - l, c.y - h), col);
	dl->AddTriangleFilled(ImVec2(c.x - s, c.y), ImVec2(c.x - s + l, c.y - h), ImVec2(c.x - s + l, c.y + h), col);
	dl->AddTriangleFilled(ImVec2(c.x, c.y - s), ImVec2(c.x + h, c.y - s + l), ImVec2(c.x - h, c.y - s + l), col);
	dl->AddTriangleFilled(ImVec2(c.x, c.y + s), ImVec2(c.x - h, c.y + s - l), ImVec2(c.x + h, c.y + s - l), col);
}

static void IconRotate(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float rad = r * 0.5f, h = r * 0.18f;
	float a0 = IM_PI * 0.15f, a1 = IM_PI * 1.70f;
	dl->PathArcTo(c, rad, a0, a1, 24);
	dl->PathStroke(col, 0, 1.6f);

	// Pointe de flèche au bout de l'arc
	ImVec2 n(cosf(a1), sinf(a1));  // vers l'extérieur
	ImVec2 t(-n.y, n.x);           // tangente (sens de l'arc)
	ImVec2 p(c.x + n.x * rad, c.y + n.y * rad);
	dl->AddTriangleFilled(ImVec2(p.x + t.x * h * 1.6f, p.y + t.y * h * 1.6f),
		ImVec2(p.x + n.x * h, p.y + n.y * h), ImVec2(p.x - n.x * h, p.y - n.y * h), col);
}

static void IconScale(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
{
	float s = r * 0.55f, b = s * 0.55f;
	dl->AddRect(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), col, 1.5f, 0, 1.5f);
	dl->AddRectFilled(ImVec2(c.x - s, c.y + s - b), ImVec2(c.x - s + b, c.y + s), col);
	dl->AddLine(ImVec2(c.x - s + b, c.y + s - b), ImVec2(c.x + s * 0.6f, c.y - s * 0.6f), col, 1.5f);
}

#define PROFILE_FUNCTION(name) Timer timer##__LINE__(name, [&](ProfileResult _result) {m_ProfileResults.push_back(_result); })

namespace Mayhem
{
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
		Fn m_Func;
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped = false;
	};


	EditorLayer::EditorLayer() : Layer("Sandbox2D")
	{
		m_ContentBrowser.SetContext(this);
	}

	void EditorLayer::OnAttach()
	{
		AssetsManager::Init("assets");

		m_Texture = Texture2D::Create("assets/textures/Checkerboard.png");
		m_AlphaTexture = Texture2D::Create("assets/textures/AlphaCheckerboard.png");
		//m_SpriteSheet = Texture2D::Create("assets/game/Spritesheet.png");
		//m_Sprites = SubTexture2D::CreateAllSpriteSheet(m_SpriteSheet, { 64.f,64.f });

		FrameBufferSpecification spec;
		spec.Attachment = { FrameBufferTextureFormat::RGBA8, FrameBufferTextureFormat::RED_INTEGER, FrameBufferTextureFormat::Depth };
		spec.Width = 1280;
		spec.Height = 720;
		m_FrameBuffer = FrameBuffer::Create(spec);

		NewScene();

		m_EditorCamera = EditorCamera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

		m_Hierarchy.SetContext(m_ActiveScene);
		MarkSceneSaved();

		m_Hierarchy.SetContentBrowser(&m_ContentBrowser);
	}

	void EditorLayer::OnDetach()
	{
		ThumbnailRenderer::Shutdown();
	}

	void EditorLayer::OnUpdate(Timestep _timestep)
	{
		{
			PROFILE_FUNCTION("Editor::OnUpdate");

			// Resize
			FrameBufferSpecification spec = m_FrameBuffer->GetSpecification();
			if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
				(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
			{
				m_FrameBuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);

				m_ActiveScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			}


			// Update


		}
		// Statistics
		SceneRenderer::ResetStats();
		// Render
		{
			PROFILE_FUNCTION("Editor::OnRender");
			m_FrameBuffer->Bind();
			RenderCommand::Clear(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

			m_FrameBuffer->ClearAttachement(1, -1);

			std::vector<int> outlined;
			if (m_SceneState == SceneState::Editor)
			{
				Entity selected = m_Hierarchy.GetSelectedEntity();
				if (selected)
					CollectOutlinedEntities(selected, outlined);
			}
			SceneRenderer::SetOutline(outlined, { 0.70f, 0.35f, 1.0f, 1.0f }, 2.0f);
			SceneRenderer::SetGridEnabled(m_SceneState == SceneState::Editor && m_ShowGrid);
			SceneRenderer::SetWireframe(m_ShowWireframe);

			// Update Scene
			if (m_SceneState == SceneState::Editor)
			{
				m_EditorCamera.OnUpdate(_timestep);

				m_ActiveScene->OnUpdateEditor(_timestep, m_EditorCamera);
			}
			else if (m_SceneState == SceneState::Runtime)
			{
				m_ActiveScene->OnUpdateRuntime(_timestep);
			}
		}
		PROFILE_FUNCTION("Editor::ReadFrameBufferPixel");
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		auto viewportW = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
		auto viewportH = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
		my = viewportH - my;

		int mouseX = (int)mx;
		int mouseY = (int)my;
		if (mouseX >= 0 && mouseY >= 00 && mouseX < (int)viewportW && mouseY < (int)viewportH)
		{
			if (m_ViewportHovered)
			{
				int id = m_FrameBuffer->ReadPixel(1, mouseX, mouseY);
				if (id == -1)
				{
					m_HoveredEntity = { entt::null, m_ActiveScene.get() };
				}
				else
				{
					Entity hovered = (id == -1) ? Entity() : Entity((entt::entity)id, m_ActiveScene.get());
					m_HoveredEntity = hovered.IsValid() ? hovered : Entity();
				}
			}
		}

		m_FrameBuffer->Unbind();
	}

	void EditorLayer::UI_Toolbar()
	{
		const float height = 36.0f;
		const float buttonSize = 26.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));

		// Une "child" de hauteur fixe dans la fenêtre de la dockspace : ni dockable, ni redimensionnable
		ImGui::BeginChild("##Toolbar", ImVec2(0.0f, height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		// Nom de la scène à gauche, avec * si modifiée
		std::string title = GetSceneDisplayName() + (m_SceneDirty ? " *" : "");
		ImGui::SetCursorPos(ImVec2(12.0f, (height - ImGui::GetTextLineHeight()) * 0.5f));
		if (m_SceneDirty)
			ImGui::Text("%s", title.c_str());
		else
			ImGui::TextDisabled("%s", title.c_str());

		// Centré horizontalement comme dans Unity (ajuste buttonCount si tu ajoutes Pause / Step)
		const int buttonCount = 1;
		const float groupWidth = buttonCount * buttonSize + (buttonCount - 1) * 4.0f;
		ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth() - groupWidth) * 0.5f, (height - buttonSize) * 0.5f));

		const bool playing = m_SceneState == SceneState::Runtime;
		if (ToolButton("##Play", playing, buttonSize, playing ? IconStop : IconPlay, playing ? "Stop" : "Play"))
		{
			if (playing)
				OnSceneStop();
			else
				OnScenePlay();
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	void EditorLayer::UI_GizmoOverlay()
	{
		const float buttonSize = 26.0f, padding = 4.0f, spacing = 2.0f;

		struct Tool { const char* Id; int Type; IconDrawer Icon; const char* Tooltip; };
		const Tool tools[] = {
			{ "##Select", -1,                              IconSelect, "Selection (Q)" },
			{ "##Move",   ImGuizmo::OPERATION::TRANSLATE,  IconMove,   "Deplacer (W)" },
			{ "##Rotate", ImGuizmo::OPERATION::ROTATE,     IconRotate, "Pivoter (E)" },
			{ "##Scale",  ImGuizmo::OPERATION::SCALE,      IconScale,  "Redimensionner (R)" },
		};
		const int count = IM_ARRAYSIZE(tools);

		// Coin haut-gauche de la zone du viewport
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
		ImGui::SetCursorScreenPos(ImVec2(winPos.x + contentMin.x + 10.0f, winPos.y + contentMin.y + 10.0f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.11f, 0.85f));

		const int buttonCount = count + 3; // outils du gizmo + mode local/world + grille + arêtes
		ImVec2 size(buttonCount * buttonSize + (buttonCount - 1) * spacing + 2.0f * padding, buttonSize + 2.0f * padding);
		ImGui::BeginChild("##GizmoOverlay", size, false,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysUseWindowPadding);

		for (int i = 0; i < count; i++)
		{
			if (i > 0)
				ImGui::SameLine();

			if (ToolButton(tools[i].Id, m_GizmoType == tools[i].Type, buttonSize, tools[i].Icon, tools[i].Tooltip))
				m_GizmoType = tools[i].Type;
		}

		ImGui::SameLine();
		const bool local = m_GizmoMode == ImGuizmo::MODE::LOCAL;
		if (ToolButton("##GizmoMode", false, buttonSize, local ? IconLocal : IconWorld,
			local ? "Local : axes de l'objet (X)" : "World : axes du monde (X)"))
		{
			m_GizmoMode = local ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
		}

		ImGui::SameLine();
		if (ToolButton("##Grid", m_ShowGrid, buttonSize, IconGrid, "Grille (G)"))
			m_ShowGrid = !m_ShowGrid;

		ImGui::SameLine();
		if (ToolButton("##Wireframe", m_ShowWireframe, buttonSize, IconWireframe, "Arretes des modeles (wireframe)"))
			m_ShowWireframe = !m_ShowWireframe;

		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
	}

	void EditorLayer::OnImGuiRender()
	{
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.



		bool open = true;
		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			UI_Toolbar();
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = 30.0f;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl + N"))
					RequestSceneAction(PendingSceneAction::New);

				if (ImGui::MenuItem("Open...", "Ctrl + O"))
					RequestSceneAction(PendingSceneAction::Open);

				ImGui::Separator();

				if (ImGui::MenuItem("Save", "Ctrl + S", false, m_SceneState == SceneState::Editor))
					SaveScene();

				if (ImGui::MenuItem("Save as...", "Ctrl + Shift + S", false, m_SceneState == SceneState::Editor))
					SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					RequestSceneAction(PendingSceneAction::Exit);

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		m_Hierarchy.OnImGuiRender();
		m_ContentBrowser.OnImGuiRender();

		ImGui::Begin("Profiling");
		SceneRenderer::Statistics stats = SceneRenderer::GetStats();
		ImGui::Text("Renderer Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Meshes: %d", stats.MeshCount);
		ImGui::Text("Quads: %d", stats.QuadCount);

		ImGui::Text("Culled: %d", stats.CulledCount);
		bool culling = SceneRenderer::IsFrustumCullingEnabled();
		if (ImGui::Checkbox("Frustum Culling", &culling))
			SceneRenderer::SetFrustumCulling(culling);

		ImGui::Separator();
		for (ProfileResult& result : m_ProfileResults)
			ImGui::Text("  %.3fms %s", result.Time, result.Name);
		m_ProfileResults.clear();
		ImGui::Separator();
		for (ProfileResult& result : m_ProfileResults)
		{
			char label[75];
			strcpy_s(label, sizeof(label), "  %.3fms ");
			strcat_s(label, sizeof(label), result.Name);
			ImGui::Text(label, result.Time);
		}
		m_ProfileResults.clear();
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
		m_EditorCamera.SetViewportHovered(m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		uint32_t textureID = m_FrameBuffer->GetColorAttachmentRendererID();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		ImGui::Image((ImTextureID)textureID, ImVec2{ viewportPanelSize.x,viewportPanelSize.y }, ImVec2{ 0,1 }, ImVec2{ 1,0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;

				std::string extension = GetExtension(std::filesystem::path(path).string());

				if (extension == ".mayhem")
					RequestSceneAction(PendingSceneAction::OpenPath, path);
				else if (extension == Prefab::Extension)
				{
					InstantiatePrefab(path);
				}
				else if (extension == ".fbx")
				{
					Entity ent = m_ActiveScene->CreateEntity(std::filesystem::path(path).filename().string());
					MeshRenderer& renderer = ent.AddComponent<MeshRenderer>();
					renderer.m_MeshPath = std::filesystem::path(path).string();
				}
				else if (extension == ".png")
				{
					Entity ent = m_ActiveScene->CreateEntity(std::filesystem::path(path).filename().string());
					SpriteRenderer& renderer = ent.AddComponent<SpriteRenderer>();
					Ref<Texture2D> text = Texture2D::Create(std::filesystem::path(path).string());
					renderer.m_Texture = text;
				}
			}

			ImGui::EndDragDropTarget();
		}

		// Icônes des objets invisibles (caméra, lumière, ciel) + aides de l'objet sélectionné
		m_HoveredIcon = {};
		if (m_SceneState == SceneState::Editor)
		{
			m_HoveredIcon = m_ViewportIcons.Draw(m_ActiveScene, m_EditorCamera.GetViewProjection(),
				ImVec2(m_ViewportBounds[0].x, m_ViewportBounds[0].y),
				ImVec2(m_ViewportBounds[1].x, m_ViewportBounds[1].y),
				m_Hierarchy.GetSelectedEntity());
		}

		// Gizmos
		Entity selected = m_Hierarchy.GetSelectedEntity();
		if (selected && m_SceneState == SceneState::Editor)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

			// Débloque le gizmo une fois le clic de sélection relâché
			if (m_BlockGizmoUntilRelease && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
				m_BlockGizmoUntilRelease = false;

			ImGuizmo::Enable(!m_BlockGizmoUntilRelease);

			// Camera
			if (m_GizmoType != -1)
			{
				const glm::mat4& camProj = m_EditorCamera.GetProjection();
				glm::mat4 camView = m_EditorCamera.GetViewMatrix();

				// Entity transform
				auto& tc = selected.GetComponent<TransformComponent>();
				glm::mat4 transform = selected.GetWorldTransform();

				// Snapping
				bool snap = Input::IsKeyPressed(Key::LeftControl);
				float snapValue = 0.5f;
				if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
					snapValue = 45.f;

				float snapValues[3] = { snapValue, snapValue , snapValue };

				// Mémorise l'état "avant" dès le clic sur le gizmo (avant qu'il ne déplace l'objet)
				if (!ImGuizmo::IsUsing() && ImGuizmo::IsOver() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					m_Undo.BeginInteraction(m_ActiveScene);

				ImGuizmo::Manipulate(glm::value_ptr(camView), glm::value_ptr(camProj),
					(ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)m_GizmoMode, glm::value_ptr(transform),
					nullptr, snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					// Monde -> local : on retire la transform du parent
					if (Entity parent = selected.GetParent())
						transform = glm::inverse(parent.GetWorldTransform()) * transform;

					glm::vec3 translation, rotation, scale;
					Maths::DecomposeTransform(transform, translation, rotation, scale);

					glm::vec3 deltaRotation = rotation - tc.m_Rotation;

					tc.m_Translation = translation;
					tc.m_Rotation += deltaRotation;
					tc.m_Scale = scale;
				}
			}
		}

		UI_GizmoOverlay();

		ImGui::End();
		ImGui::PopStyleVar();

		// ---------- Annuler / Rétablir ----------
		ImGuiIO& undoIO = ImGui::GetIO();
		if (m_SceneState == SceneState::Editor)
		{
			// Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z (pas pendant la saisie de texte : l'InputText garde son propre Ctrl+Z)
			if (undoIO.KeyCtrl && !undoIO.WantTextInput && !ImGui::IsAnyItemActive())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
				{
					if (undoIO.KeyShift)
						OnRedo();
					else
						OnUndo();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
				{
					OnRedo();
				}
			}

			// Un clic/glisser/saisie en cours, ou le gizmo en train d'être utilisé
			bool interacting = ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();
			m_Undo.Track(m_ActiveScene, interacting);
		}
		UpdateDirtyState();
		UI_UnsavedChangesPopup();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& _e)
	{
		m_EditorCamera.OnEvent(_e);

		EventDispatcher dispatcher(_e);
		dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(ENGINE_BIND_EVENT_FN(EditorLayer::OnMousePressed));
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& _e)
	{
		// Shortcuts
		if (_e.GetRepeatCount() > 0)
			return false;

		if (m_EditorCamera.IsManipulating())
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		switch (_e.GetKeyCode())
		{
		case Key::N:
			if (control)
				RequestSceneAction(PendingSceneAction::New);
			break;
		case Key::O:
			if (control)
				RequestSceneAction(PendingSceneAction::Open);
			break;
		case Key::S:
			if (control)
			{
				if (shift)
					SaveSceneAs();
				else
					SaveScene();
			}
			break;
		case Key::D:
			if (control)
				OnDuplicateEntity();
			break;
		case Key::G:
			m_ShowGrid = !m_ShowGrid;
			break;
		case Key::X:
			m_GizmoMode = (m_GizmoMode == ImGuizmo::MODE::LOCAL) ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
			break;
			// Gizmos
		case Key::Q:
			m_GizmoType = -1;
			break;
		case Key::W:
			m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			m_GizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		case Key::F:
		{
			Entity selected = m_Hierarchy.GetSelectedEntity();
			if (selected && m_SceneState == SceneState::Editor)
			{
				glm::mat4 world = selected.GetWorldTransform();
				glm::vec3 target = glm::vec3(world[3]);
				float distance = 5.0f;

				// Si l'objet a un mesh : on cadre sa taille réelle
				if (selected.HasComponent<MeshRenderer>())
				{
					auto& mr = selected.GetComponent<MeshRenderer>();
					if (mr.m_Mesh)
					{
						WorldBounds bounds = WorldBounds::FromLocal(mr.m_Mesh->GetBounds(), world);
						target = bounds.Center;
						distance = glm::length(bounds.Extents) * 2.5f;
					}
				}

				m_EditorCamera.Focus(target, distance);
			}
			break;
		}
		default:
			break;
		}

		return false;
	}

	bool EditorLayer::OnMousePressed(MouseButtonPressedEvent& _e)
	{
		if (ImGuizmo::GetHoveredHandleType() == ImGuizmo::MOVETYPE::MT_NONE)
		{
			if (!Mayhem::Input::IsKeyPressed(Mayhem::Key::LeftAlt) && m_ViewportHovered)
			{
				if (_e.GetMouseButton() == Mayhem::Mouse::Button0)
				{
					Entity clicked = m_HoveredIcon ? m_HoveredIcon : m_HoveredEntity;
					Entity current = m_Hierarchy.GetSelectedEntity();

					// Nouvelle sélection : le gizmo apparaîtra sous la souris, on l'ignore jusqu'au relâchement
					if (clicked != current)
						m_BlockGizmoUntilRelease = true;

					m_Hierarchy.SetSelectedEntity(clicked);
				}
			}
		}

		return false;
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_SceneState == SceneState::Runtime)
			return;

		m_Undo.CancelInteraction();
		uint64_t selected = m_Hierarchy.GetSelectedEntity() ? (uint64_t)m_Hierarchy.GetSelectedEntity().GetUUID() : 0;

		// On joue sur une copie : la scène éditée n'est jamais modifiée par le jeu
		m_EditorScene = m_ActiveScene;
		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		m_SceneState = SceneState::Runtime;

		m_Hierarchy.SetContext(m_ActiveScene);
		m_Hierarchy.SetSelectedEntity(m_ActiveScene->GetEntityByUUID(selected)); // garde la sélection
	}

	void EditorLayer::OnSceneStop()
	{
		if (m_SceneState == SceneState::Editor)
			return;

		uint64_t selected = m_Hierarchy.GetSelectedEntity() ? (uint64_t)m_Hierarchy.GetSelectedEntity().GetUUID() : 0;

		// On jette la copie jouée et on revient à la scène d'origine
		m_ActiveScene->OnRuntimeStop();
		m_ActiveScene = m_EditorScene;
		m_EditorScene = nullptr;
		m_ActiveScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		m_SceneState = SceneState::Editor;

		m_Hierarchy.SetContext(m_ActiveScene);
		m_Hierarchy.SetSelectedEntity(m_ActiveScene->GetEntityByUUID(selected));
	}

	void EditorLayer::NewScene()
	{
		if (m_SceneState == SceneState::Runtime)
			OnSceneStop();

		m_ActiveScene = MakeRef<Scene>();
		m_ActiveScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_Hierarchy.SetContext(m_ActiveScene);
		m_EditorScenePath.clear();
		m_Undo.Clear();

		Entity sky = m_ActiveScene->CreateEntity("Skybox");
		sky.AddComponent<Skybox>();
		Entity light = m_ActiveScene->CreateEntity("Directional Light");
		light.AddComponent<DirectionalLight>();
		light.GetComponent<TransformComponent>().m_Translation = { 0.f, 1.f, 0.f };
		light.GetComponent<TransformComponent>().m_Rotation = { glm::radians(0.f), glm::radians(45.f), glm::radians(45.f) };
		Entity camera = m_ActiveScene->CreateEntity("Main camera");
		CameraComponent& cam = camera.AddComponent<CameraComponent>();
		cam.m_Camera.SetProjectionType(SceneCamera::ProjectionType::Perspective);
		camera.GetComponent<TransformComponent>().m_Translation = { 0.f, 0.f, 2.f };
		MarkSceneSaved();
	}

	void EditorLayer::OpenScene()
	{
		std::string path = FileDialogs::OpenFile("Mayhem Scene (*.mayhem)\0*.mayhem\0");
		if (!path.empty())
			OpenScene(path);
	}

	void EditorLayer::OpenScene(const std::filesystem::path& _path)
	{
		if (m_SceneState == SceneState::Runtime)
			OnSceneStop();

		Ref<Scene> newScene = MakeRef<Scene>();
		newScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		SceneSerializer serializer(newScene);
		if (!serializer.Deserialize(_path.string()))
			return; // fichier invalide : on garde la scène actuelle

		m_ActiveScene = newScene;
		m_Hierarchy.SetContext(m_ActiveScene);
		m_EditorScenePath = _path;
		m_Undo.Clear();
		MarkSceneSaved();
	}

	void EditorLayer::SaveScene()
	{
		if (m_SceneState == SceneState::Runtime)
			return; // on ne sauvegarde pas l'état du jeu

		if (m_EditorScenePath.empty())
		{
			SaveSceneAs();
			return;
		}

		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(m_EditorScenePath.string());
		MarkSceneSaved();
	}

	void EditorLayer::SaveSceneAs()
	{
		if (m_SceneState == SceneState::Runtime)
			return;

		std::string path = FileDialogs::SaveFile("Mayhem Scene (*.mayhem)\0*.mayhem\0");
		if (path.empty())
			return;

		SceneSerializer serializer(m_ActiveScene);
		serializer.Serialize(path);
		m_EditorScenePath = path;
		MarkSceneSaved();
	}

	void EditorLayer::OnDuplicateEntity()
	{
		Entity selected = m_Hierarchy.GetSelectedEntity();
		if (!selected)
			return;

		m_Undo.Record(m_ActiveScene);
		m_Hierarchy.SetSelectedEntity(m_ActiveScene->DuplicateEntity(selected));
	}

	void EditorLayer::OnUndo()
	{
		if (m_SceneState != SceneState::Editor)
			return;

		if (Ref<Scene> scene = m_Undo.Undo(m_ActiveScene))
			SetEditorScene(scene);
	}

	void EditorLayer::OnRedo()
	{
		if (m_SceneState != SceneState::Editor)
			return;

		if (Ref<Scene> scene = m_Undo.Redo(m_ActiveScene))
			SetEditorScene(scene);
	}

	// Remplace la scène affichée en gardant la sélection (par UUID)
	void EditorLayer::SetEditorScene(const Ref<Scene>& _scene)
	{
		uint64_t selected = m_Hierarchy.GetSelectedEntity() ? (uint64_t)m_Hierarchy.GetSelectedEntity().GetUUID() : 0;

		m_ActiveScene = _scene;
		m_ActiveScene->OnViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

		m_Hierarchy.SetContext(m_ActiveScene);
		m_Hierarchy.SetSelectedEntity(m_ActiveScene->GetEntityByUUID(selected));
		m_HoveredEntity = Entity(entt::null, m_ActiveScene.get());
	}

	Entity EditorLayer::InstantiatePrefab(const std::filesystem::path& _path, Entity _parent)
	{
		Entity instance = Prefab::Instantiate(*m_ActiveScene, _path.lexically_normal().generic_string(), _parent);
		if (instance)
			m_Hierarchy.SetSelectedEntity(instance);
		return instance;
	}

	std::string EditorLayer::GetSceneDisplayName() const
	{
		return m_EditorScenePath.empty() ? "Sans titre" : m_EditorScenePath.filename().string();
	}

	void EditorLayer::MarkSceneSaved()
	{
		SceneSerializer serializer(m_ActiveScene);
		m_SavedSceneText = serializer.SerializeToString();
		m_SceneDirty = false;
		m_CheckedVersion = m_Undo.GetVersion();
	}

	// Ne recompare la scène que si l'historique a bougé (modification, Ctrl+Z, Ctrl+Y)
	void EditorLayer::UpdateDirtyState()
	{
		if (m_SceneState != SceneState::Editor)
			return;

		const uint64_t version = m_Undo.GetVersion();
		if (version == m_CheckedVersion)
			return;

		m_CheckedVersion = version;
		SceneSerializer serializer(m_ActiveScene);
		m_SceneDirty = serializer.SerializeToString() != m_SavedSceneText; // revenu à l'état sauvegardé = propre
	}

	// Nouvelle scène / ouverture / quitter : demande d'abord s'il faut sauvegarder
	void EditorLayer::RequestSceneAction(PendingSceneAction _action, const std::filesystem::path& _path)
	{
		if (m_SceneState == SceneState::Runtime)
			OnSceneStop();

		UpdateDirtyState();
		if (!m_SceneDirty)
		{
			ExecuteSceneAction(_action, _path);
			return;
		}

		m_PendingAction = _action;
		m_PendingPath = _path;
		m_OpenUnsavedPopup = true;
	}

	void EditorLayer::ExecuteSceneAction(PendingSceneAction _action, const std::filesystem::path& _path)
	{
		switch (_action)
		{
		case PendingSceneAction::New:      NewScene(); break;
		case PendingSceneAction::Open:     OpenScene(); break;
		case PendingSceneAction::OpenPath: OpenScene(_path); break;
		case PendingSceneAction::Exit:     Application::Get().Close(); break;
		default: break;
		}
	}

	void EditorLayer::UI_UnsavedChangesPopup()
	{
		const char* popupName = "Modifications non enregistrees";

		if (m_OpenUnsavedPopup)
		{
			ImGui::OpenPopup(popupName);
			m_OpenUnsavedPopup = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("La scene \"%s\" a ete modifiee.", GetSceneDisplayName().c_str());
			ImGui::Text("Voulez-vous enregistrer les modifications ?");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			auto finish = [this](bool _execute)
				{
					ImGui::CloseCurrentPopup();
					PendingSceneAction action = m_PendingAction;
					std::filesystem::path path = m_PendingPath;
					m_PendingAction = PendingSceneAction::None;
					m_PendingPath.clear();
					if (_execute)
						ExecuteSceneAction(action, path);
				};

			if (ImGui::Button("Enregistrer", ImVec2(130, 0)))
			{
				SaveScene();
				if (!m_SceneDirty) // "Enregistrer sous" annulé = on reste là
					finish(true);
			}
			ImGui::SameLine();
			if (ImGui::Button("Ne pas enregistrer", ImVec2(130, 0)))
				finish(true);
			ImGui::SameLine();
			if (ImGui::Button("Annuler", ImVec2(130, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
				finish(false);

			ImGui::EndPopup();
		}
	}
}
