#pragma once

#include <Engine/Core/Input.h>

namespace Mayhem
{
	class WindowsInput : public Input
	{
	protected:
		virtual bool IsKeyPressedImpl(int _key) override;
		virtual bool IsMouseButtonPressedImpl(int _button) override;
		virtual float GetMouseXImpl() override;
		virtual float GetMouseYImpl() override;
		virtual std::pair<float, float> GetMousePositionImpl() override;
	};
}