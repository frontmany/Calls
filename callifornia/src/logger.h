#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <iostream>

inline std::shared_ptr<spdlog::logger> getLogger()
{
	static std::shared_ptr<spdlog::logger> logger = nullptr;
	
	if (!logger)
	{
		try
		{
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::debug);

			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				"logs/callifornia.log", 1024 * 1024 * 10, 3);
			file_sink->set_level(spdlog::level::trace);

			logger = std::make_shared<spdlog::logger>("callifornia",
				spdlog::sinks_init_list{ console_sink, file_sink });
			
			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::info);
			
			spdlog::register_logger(logger);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			std::cerr << "Log initialization failed: " << ex.what() << std::endl;
		}
	}
	
	return logger;
}

#define LOG_TRACE(...) if (auto logger = getLogger()) { logger->trace(__VA_ARGS__); }
#define LOG_DEBUG(...) if (auto logger = getLogger()) { logger->debug(__VA_ARGS__); }
#define LOG_INFO(...)  if (auto logger = getLogger()) { logger->info(__VA_ARGS__); }
#define LOG_WARN(...)  if (auto logger = getLogger()) { logger->warn(__VA_ARGS__); }
#define LOG_ERROR(...) if (auto logger = getLogger()) { logger->error(__VA_ARGS__); }










