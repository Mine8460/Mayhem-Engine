#pragma once

#include <Enpch.h>

#include <Engine/Core/Core.h>
#include <Engine/Events/Event.h>

namespace Mayhem
{
	struct WindowProps
	{
		std::string title;
		unsigned int width;
		unsigned int height;

		WindowProps(const std::string& _title = "Engine",
			unsigned int _width = 1280,
			unsigned int _height = 720)
			: title(_title), width(_width), height(_height)
		{
		}
	};

	// Just an interface, have to be herited for every platform
	class ENGINE_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {};

		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& _callback) = 0;
		virtual void SetVSync(bool _enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Window* Create(std::string _name, const WindowProps& props = WindowProps());
	};
}
