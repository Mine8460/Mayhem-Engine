#include <Enpch.h>
#include "ImGuiLayer.h"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include <Engine/Core/Application.h>

namespace Mayhem
{

	ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Flags setup
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Keyboard controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Gamepad controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 18.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Medium.ttf", 18.0f);

		// Setup style
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		SetDarkThemColors();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version  410");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& _e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			_e.Handled |= _e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			_e.Handled |= _e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::SetDarkThemColors()
	{
		auto& colors = ImGui::GetStyle().Colors;

		//// Window
		//colors[ImGuiCol_WindowBg] = ImVec4{ 0.043f, 0.039f, 0.059f, 1.0f };

		//// Headers
		//colors[ImGuiCol_Header] = ImVec4{ 0.094f, 0.082f, 0.122f, 1.0f };
		//colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.145f, 0.125f, 0.184f, 1.0f };
		//colors[ImGuiCol_HeaderActive] = ImVec4{ 0.188f, 0.149f, 0.239f, 1.0f };

		//// Buttons
		//colors[ImGuiCol_Button] = ImVec4{ 0.094f, 0.082f, 0.122f, 1.0f };
		//colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };
		//colors[ImGuiCol_ButtonActive] = ImVec4{ 0.443f, 0.227f, 0.569f, 1.0f };

		//// Frame BG
		//colors[ImGuiCol_FrameBg] = ImVec4{ 0.082f, 0.075f, 0.102f, 1.0f };
		//colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.129f, 0.110f, 0.161f, 1.0f };
		//colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.173f, 0.141f, 0.212f, 1.0f };

		//// Tabs
		//colors[ImGuiCol_Tab] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		//colors[ImGuiCol_TabHovered] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };
		//colors[ImGuiCol_TabActive] = ImVec4{ 0.227f, 0.161f, 0.282f, 1.0f };
		//colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		//colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.122f, 0.102f, 0.157f, 1.0f };

		//// Title
		//colors[ImGuiCol_TitleBg] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		//colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.094f, 0.075f, 0.122f, 1.0f };
		//colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };

		// ============================================================
// MAYHEM ENGINE - ImGui Theme
// ============================================================

// Main window
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.043f, 0.039f, 0.059f, 1.0f };

		// ============================================================
		// Headers
		// ============================================================

		colors[ImGuiCol_Header] = ImVec4{ 0.094f, 0.082f, 0.122f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.145f, 0.125f, 0.184f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.188f, 0.149f, 0.239f, 1.0f };

		// ============================================================
		// Buttons
		// ============================================================

		colors[ImGuiCol_Button] = ImVec4{ 0.094f, 0.082f, 0.122f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.443f, 0.227f, 0.569f, 1.0f };

		// ============================================================
		// Frame Background
		// ============================================================

		colors[ImGuiCol_FrameBg] = ImVec4{ 0.082f, 0.075f, 0.102f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.129f, 0.110f, 0.161f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.173f, 0.141f, 0.212f, 1.0f };

		// ============================================================
		// Tabs
		// ============================================================

		colors[ImGuiCol_Tab] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.227f, 0.161f, 0.282f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.122f, 0.102f, 0.157f, 1.0f };

		// ============================================================
		// Title Bar
		// ============================================================

		colors[ImGuiCol_TitleBg] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.094f, 0.075f, 0.122f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.071f, 0.063f, 0.086f, 1.0f };

		// ============================================================
		// Menu
		// ============================================================

		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.055f, 0.047f, 0.071f, 1.0f };
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.059f, 0.051f, 0.075f, 0.98f };

		// ============================================================
		// Borders
		// ============================================================

		colors[ImGuiCol_Border] = ImVec4{ 0.180f, 0.145f, 0.220f, 0.55f };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.000f, 0.000f, 0.000f, 0.00f };

		// ============================================================
		// Scrollbars
		// ============================================================

		colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.035f, 0.031f, 0.043f, 1.0f };
		colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.145f, 0.125f, 0.165f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.250f, 0.200f, 0.300f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };

		// ============================================================
		// Checkboxes / Sliders
		// ============================================================

		colors[ImGuiCol_CheckMark] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };

		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.443f, 0.227f, 0.569f, 1.0f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };

		// ============================================================
		// Separators
		// ============================================================

		colors[ImGuiCol_Separator] = ImVec4{ 0.180f, 0.145f, 0.220f, 0.55f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.608f, 0.302f, 0.792f, 0.78f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };

		// ============================================================
		// Text
		// ============================================================

		colors[ImGuiCol_Text] = ImVec4{ 0.900f, 0.890f, 0.920f, 1.0f };
		colors[ImGuiCol_TextDisabled] = ImVec4{ 0.450f, 0.430f, 0.480f, 1.0f };

		// ============================================================
		// Selection
		// ============================================================

		colors[ImGuiCol_TextSelectedBg] = ImVec4{ 0.608f, 0.302f, 0.792f, 0.35f };

		// ============================================================
		// Drag & Drop
		// ============================================================

		colors[ImGuiCol_DragDropTarget] = ImVec4{ 0.950f, 0.250f, 0.550f, 0.90f };

		// ============================================================
		// Navigation
		// ============================================================

		colors[ImGuiCol_NavHighlight] = ImVec4{ 0.608f, 0.302f, 0.792f, 1.0f };
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4{ 0.608f, 0.302f, 0.792f, 0.70f };
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4{ 0.000f, 0.000f, 0.000f, 0.20f };

		// ============================================================
		// Modal
		// ============================================================

		colors[ImGuiCol_ModalWindowDimBg] = ImVec4{ 0.000f, 0.000f, 0.000f, 0.55f };

		auto& style = ImGui::GetStyle();

		style.WindowRounding = 4.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 3.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 4.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 3.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
	}

	ImGuiKey ImGuiLayer::ConvertGLFWInputToImGui(int _key)
	{
		switch (_key)
		{
		case GLFW_KEY_TAB: return ImGuiKey_Tab;
		case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
		case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
		case GLFW_KEY_UP: return ImGuiKey_UpArrow;
		case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
		case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
		case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
		case GLFW_KEY_HOME: return ImGuiKey_Home;
		case GLFW_KEY_END: return ImGuiKey_End;
		case GLFW_KEY_INSERT: return ImGuiKey_Insert;
		case GLFW_KEY_DELETE: return ImGuiKey_Delete;
		case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
		case GLFW_KEY_SPACE: return ImGuiKey_Space;
		case GLFW_KEY_ENTER: return ImGuiKey_Enter;
		case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
		case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
		case GLFW_KEY_COMMA: return ImGuiKey_Comma;
		case GLFW_KEY_MINUS: return ImGuiKey_Minus;
		case GLFW_KEY_PERIOD: return ImGuiKey_Period;
		case GLFW_KEY_SLASH: return ImGuiKey_Slash;
		case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
		case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
		case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
		case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
		case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
		case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
		case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
		case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
		case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
		case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
		case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
		case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
		case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
		case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
		case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
		case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
		case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
		case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
		case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
		case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
		case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
		case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
		case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
		case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
		case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
		case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
		case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
		case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
		case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
		case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
		case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
		case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
		case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
		case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
		case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
		case GLFW_KEY_MENU: return ImGuiKey_Menu;
		case GLFW_KEY_0: return ImGuiKey_0;
		case GLFW_KEY_1: return ImGuiKey_1;
		case GLFW_KEY_2: return ImGuiKey_2;
		case GLFW_KEY_3: return ImGuiKey_3;
		case GLFW_KEY_4: return ImGuiKey_4;
		case GLFW_KEY_5: return ImGuiKey_5;
		case GLFW_KEY_6: return ImGuiKey_6;
		case GLFW_KEY_7: return ImGuiKey_7;
		case GLFW_KEY_8: return ImGuiKey_8;
		case GLFW_KEY_9: return ImGuiKey_9;
		case GLFW_KEY_A: return ImGuiKey_A;
		case GLFW_KEY_B: return ImGuiKey_B;
		case GLFW_KEY_C: return ImGuiKey_C;
		case GLFW_KEY_D: return ImGuiKey_D;
		case GLFW_KEY_E: return ImGuiKey_E;
		case GLFW_KEY_F: return ImGuiKey_F;
		case GLFW_KEY_G: return ImGuiKey_G;
		case GLFW_KEY_H: return ImGuiKey_H;
		case GLFW_KEY_I: return ImGuiKey_I;
		case GLFW_KEY_J: return ImGuiKey_J;
		case GLFW_KEY_K: return ImGuiKey_K;
		case GLFW_KEY_L: return ImGuiKey_L;
		case GLFW_KEY_M: return ImGuiKey_M;
		case GLFW_KEY_N: return ImGuiKey_N;
		case GLFW_KEY_O: return ImGuiKey_O;
		case GLFW_KEY_P: return ImGuiKey_P;
		case GLFW_KEY_Q: return ImGuiKey_Q;
		case GLFW_KEY_R: return ImGuiKey_R;
		case GLFW_KEY_S: return ImGuiKey_S;
		case GLFW_KEY_T: return ImGuiKey_T;
		case GLFW_KEY_U: return ImGuiKey_U;
		case GLFW_KEY_V: return ImGuiKey_V;
		case GLFW_KEY_W: return ImGuiKey_W;
		case GLFW_KEY_X: return ImGuiKey_X;
		case GLFW_KEY_Y: return ImGuiKey_Y;
		case GLFW_KEY_Z: return ImGuiKey_Z;
		case GLFW_KEY_F1: return ImGuiKey_F1;
		case GLFW_KEY_F2: return ImGuiKey_F2;
		case GLFW_KEY_F3: return ImGuiKey_F3;
		case GLFW_KEY_F4: return ImGuiKey_F4;
		case GLFW_KEY_F5: return ImGuiKey_F5;
		case GLFW_KEY_F6: return ImGuiKey_F6;
		case GLFW_KEY_F7: return ImGuiKey_F7;
		case GLFW_KEY_F8: return ImGuiKey_F8;
		case GLFW_KEY_F9: return ImGuiKey_F9;
		case GLFW_KEY_F10: return ImGuiKey_F10;
		case GLFW_KEY_F11: return ImGuiKey_F11;
		case GLFW_KEY_F12: return ImGuiKey_F12;
		default: return ImGuiKey_None;
		}
	}
}
