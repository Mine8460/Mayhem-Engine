#pragma once

#include "Core.h"

namespace Mayhem
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
#define MAYHEM_CORE_INFO(...) Mayhem::Logger::GetInstance()->Info(__VA_ARGS__);
#define MAYHEM_CORE_WARN(...) Mayhem::Logger::GetInstance()->Warn(__VA_ARGS__);
#define MAYHEM_CORE_ERROR(...) Mayhem::Logger::GetInstance()->Error(__VA_ARGS__);
#define MAYHEM_CORE_FATAL(...) Mayhem::Logger::GetInstance()->Fatal(__VA_ARGS__);
#define MAYHEM_CORE_ASSERT(...) Mayhem::Logger::GetInstance()->Assert(__VA_ARGS__);

// Client log macros
#define LOG_INFO(...) Mayhem::Logger::GetInstance()->Info(__VA_ARGS__);
#define LOG_WARN(...) Mayhem::Logger::GetInstance()->Warn(__VA_ARGS__);
#define LOG_ERROR(...) Mayhem::Logger::GetInstance()->Error(__VA_ARGS__);
#define LOG_FATAL(...) Mayhem::Logger::GetInstance()->Fatal(__VA_ARGS__);
