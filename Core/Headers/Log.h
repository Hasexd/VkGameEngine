#pragma once

#include <string>
#include <source_location>
#include <print>

#include "Types.h"

enum class LogLevel : u8
{
	Debug,
	Info,
	Warning,
	Error,
	Critical
};

namespace
{
	void LogInternal(LogLevel level, const std::string& msg, const std::source_location& loc = std::source_location::current())
	{
		std::string levelStr;

		switch (level)
		{
		case LogLevel::Debug:
			levelStr = "[DEBUG]";
			break;
		case LogLevel::Info:
			levelStr = "[INFO]";
			break;
		case LogLevel::Warning:
			levelStr = "[WARNING]";
			break;
		case LogLevel::Error:
			levelStr = "[ERROR]";
			break;
		case LogLevel::Critical:
			levelStr = "[CRITICAL]";
			break;
		}

		std::string filePath = std::string(loc.file_name());
		size_t lastSlash = filePath.find_last_of("/\\");
		std::string fileName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);

		std::println("{} {}:{} - {}", levelStr, fileName, loc.line(), msg);
	}
}

#ifdef DEBUG

#define LOG_TRACE(...) LogInternal(LogLevel::Trace, std::format(__VA_ARGS__))
#define LOG_DEBUG(...) LogInternal(LogLevel::Debug, std::format(__VA_ARGS__))
#define LOG_INFO(...) LogInternal(LogLevel::Info, std::format(__VA_ARGS__))
#define LOG_WARN(...) LogInternal(LogLevel::Warning, std::format(__VA_ARGS__))
#define LOG_ERROR(...) LogInternal(LogLevel::Error, std::format(__VA_ARGS__))
#define LOG_CRITICAL(...) LogInternal(LogLevel::Critical, std::format(__VA_ARGS__))

#define ASSERT(condition) \
		do { \
			if (!(condition)) { \
				LOG_CRITICAL("ASSERTION FAILED: {}", #condition); \
				std::abort(); \
			} \
		} while (false)

#else

#define LOG_TRACE(...)
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_CRITICAL(...)
#define ASSERT(condition, ...)

#endif