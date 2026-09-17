#include <Enpch.h>
#include "Logger.h"
#include <cassert>

namespace Engine
{
	Logger* Logger::instance = nullptr;

	Logger* Logger::GetInstance(void)
	{
		if (instance == nullptr)
		{
			instance = new Logger();
		}
		return instance;
	}

	void Logger::Info(const char* _string)
	{
		std::cout << "INFO : " << _string << std::endl;
	}
	void Logger::Warn(const char* _string)
	{
		std::cout << "WARN : " << _string << std::endl;
	}
	void Logger::Error(const char* _string)
	{
		std::cout << "ERROR : " << _string << std::endl;
	}
	void Logger::Fatal(const char* _string)
	{
		std::cout << "FATAL : " << _string << std::endl;
	}
	void Logger::Assert(bool _value, const char* _string)
	{
		assert(_value && _string);
	}
}