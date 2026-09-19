#pragma once

#include <Engine/Core/Core.h>
#include "KeyCode.h"
#include "MouseButtonCodes.h"

namespace Mayhem
{
	class Input
	{
	public:
		inline static bool IsKeyPressed(int _key) { return s_Instance->IsKeyPressedImpl(_key); };

		inline static bool IsMouseButtonPressed(int _button) { return s_Instance->IsMouseButtonPressedImpl(_button); };
		inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); };
		inline static float GetMouseY() { return s_Instance->GetMouseYImpl(); };
		inline static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); };
	protected:
		virtual bool IsKeyPressedImpl(int _key) = 0;
		virtual bool IsMouseButtonPressedImpl(int _button) = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;
		virtual std::pair<float, float> GetMousePositionImpl() = 0;
	private:
		static Input* s_Instance;
	};
}
