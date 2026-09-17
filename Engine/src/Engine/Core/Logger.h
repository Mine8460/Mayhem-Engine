#pragma once

#include "Core.h"

namespace Engine
{
	class ENGINE_API Logger
	{
		static Logger* instance;
	public:
		static Logger* GetInstance(void);

		void Info(const char* _string);
		void Warn(const char* _string);
		void Error(const char* _string);
		void Fatal(const char* _string);
		void Assert(bool _value, const char* _string);
	};
}

// Core log macros
#define ENGINE_CORE_INFO(...) Engine::Logger::GetInstance()->Info(__VA_ARGS__);
#define ENGINE_CORE_WARN(...) Engine::Logger::GetInstance()->Warn(__VA_ARGS__);
#define ENGINE_CORE_ERROR(...) Engine::Logger::GetInstance()->Error(__VA_ARGS__);
#define ENGINE_CORE_FATAL(...) Engine::Logger::GetInstance()->Fatal(__VA_ARGS__);
#define ENGINE_CORE_ASSERT(...) Engine::Logger::GetInstance()->Assert(__VA_ARGS__);

// Client log macros
#define LOG_INFO(...) Engine::Logger::GetInstance()->Info(__VA_ARGS__);
#define LOG_WARN(...) Engine::Logger::GetInstance()->Warn(__VA_ARGS__);
#define LOG_ERROR(...) Engine::Logger::GetInstance()->Error(__VA_ARGS__);
#define LOG_FATAL(...) Engine::Logger::GetInstance()->Fatal(__VA_ARGS__);
