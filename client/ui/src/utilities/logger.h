#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <iostream>


    namespace details
    {
        inline std::shared_ptr<spdlog::logger>& get_logger_instance() noexcept
        {
            static std::shared_ptr<spdlog::logger> logger = nullptr;
            return logger;
        }

        inline bool initialize_logger() noexcept
        {
            auto& logger = get_logger_instance();
            if (logger) {
                return true;
            }

            try {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_level(spdlog::level::debug);
                console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v%$");

                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    "logs/calls_client.log",
                    1024 * 1024 * 10, 
                    3
                );
                file_sink->set_level(spdlog::level::trace);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [thread %t] %v");

                logger = std::make_shared<spdlog::logger>("calls_client",
                    spdlog::sinks_init_list{ console_sink, file_sink });

                logger->set_level(spdlog::level::trace);
                logger->flush_on(spdlog::level::info);

                spdlog::register_logger(logger);

                return true;
            }
            catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Log initialization failed: " << ex.what() << std::endl;
                return false;
            }
            catch (const std::exception& ex) {
                std::cerr << "Log initialization failed: " << ex.what() << std::endl;
                return false;
            }
        }
    }

    inline void initialize() noexcept
    {
        details::initialize_logger();
    }

    inline std::shared_ptr<spdlog::logger> get() noexcept
    {
        auto& logger = details::get_logger_instance();
        if (!logger) {
            details::initialize_logger();
        }
        return logger;
    }

    inline bool is_initialized() noexcept
    {
        return details::get_logger_instance() != nullptr;
    }

    inline void set_level(spdlog::level::level_enum level) noexcept
    {
        if (auto logger = get()) {
            logger->set_level(level);
        }
    }

    inline void flush() noexcept
    {
        if (auto logger = get()) {
            logger->flush();
        }
    }

    inline void shutdown() noexcept
    {
        if (auto logger = details::get_logger_instance()) {
            logger->flush();
            spdlog::drop(logger->name());
            details::get_logger_instance().reset();
        }
    }


#define LOG_TRACE(...) \
    do { \
        if (auto logger = get()) { \
            logger->trace(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_DEBUG(...) \
    do { \
        if (auto logger = get()) { \
            logger->debug(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(...) \
    do { \
        if (auto logger = get()) { \
            logger->info(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN(...) \
    do { \
        if (auto logger = get()) { \
            logger->warn(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR(...) \
    do { \
        if (auto logger = get()) { \
            logger->error(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_CRITICAL(...) \
    do { \
        if (auto logger = get()) { \
            logger->critical(__VA_ARGS__); \
        } \
    } while(0)