#pragma once

#ifdef ENGINE_PLATFORM_WINDOWS

extern Mayhem::Application* Mayhem::CreateApplication();

int main(int argc, char** argv)
{

	auto app = Mayhem::CreateApplication();
	app->Run();
	delete app;
}

#endif
