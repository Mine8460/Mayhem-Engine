#pragma once

// For use by application, not for Engine !

#include "Engine/Core/Application.h"

#include "Engine/Core/Input.h"
#include "Engine/Core/KeyCode.h"
#include "Engine/Core/MouseButtonCodes.h"

#include "Engine/Core/Layer.h"
#include "Engine/Core/Logger.h"

#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Core/Timestep.h"

// --- Renderer --------------------------
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Renderer2D.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Camera/OrthographicCamera.h"
// ---------------------------------------
// 
// --- Scene -----------------------------
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/ScriptableEntity.h"
// ---------------------------------------
