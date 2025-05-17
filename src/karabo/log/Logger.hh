/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *b
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_LOGCONFIG_LOGGER_HH
#define KARABO_LOGCONFIG_LOGGER_HH

// The conan package currently used by the Karabo Framework uses the default
// value "false" for "use_std_fmt", meaning that an external "fmt" library is
// assumed.
#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL
#endif

#include <spdlog/common.h>
#include <spdlog/details/log_msg_buffer.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <boost/type_index.hpp>
#include <filesystem>
#include <memory>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "LoggerStream.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "utils.hh"

#define KARABO_AUDIT_LOGGER "audit_logger"

// Support Hash formatting
template <>
struct fmt::formatter<karabo::data::Hash> : fmt::formatter<std::string> {
    auto format(const karabo::data::Hash& hash, format_context& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "\n{}", karabo::data::toString(hash));
    }
};

// Support Schema formatting
template <>
struct fmt::formatter<karabo::data::Schema> : fmt::formatter<std::string> {
    auto format(const karabo::data::Schema& schema, format_context& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "\n{}", karabo::data::toString(schema));
    }
};

// Support std::filesystem::path formatting
template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
    auto format(std::filesystem::path p, format_context& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", p.string());
    }
};

// Support karabo::data::Exception formatting
template <>
struct fmt::formatter<karabo::data::Exception> : fmt::formatter<std::string> {
    auto format(karabo::data::Exception e, format_context& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", e.detailedMsg());
    }
};

// Support std::exception formatting
template <>
struct fmt::formatter<std::exception> : fmt::formatter<std::string> {
    auto format(std::exception e, format_context& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", e.what());
    }
};


namespace karabo {

    namespace log {

        /**
         * The Logger class.
         * Configures spdlog logging system
         */
        class Logger {
            friend class LoggerStream;
            friend std::shared_ptr<spdlog::logger> details::getLogger(const std::string& name);

           public:
            KARABO_CLASSINFO(Logger, "Logger", "")

            virtual ~Logger() = default;

            static void expectedParameters(karabo::data::Schema& expected);

            /**
             * Static method allowing to configure the three appenders and the default level
             * @param config A hash which must follow the Schema described in the expectedParameters method
             */
            static void configure(const karabo::data::Hash& config);

            /**
             * Enables the cache sink for the specified logger.
             * @param name The logger name on which the sink should work (empty string reflects default logger)
             */
            static void useCache(const std::string& name = "", bool inheritSinks = true);

            /**
             * Enables the console (color) sink for the specified logger
             * @param name The logger name on which the sink should work (empty string reflects default logger)
             */
            static void useConsole(const std::string& name = "", bool inheritSinks = true);

            /**
             * Enables the rotating file sink for the specified logger.
             * @param name The logger name on which the sink should work (empty string reflects default logger)
             */
            static void useFile(const std::string& name = "", bool inheritSinks = true);

            /**
             * Enables the rolling file appender on the specified category for auditing.
             *
             * @param name The logger name for audit type messages
             */
            static void useAuditFile(const std::string& name = KARABO_AUDIT_LOGGER);

            /**
             * Resets all sinks from all loggers and destroy the cache.
             * Nothing will be logged after a call to this function.
             *
             * Use this function to re-configure logger behavior at runtime.
             */
            static void reset();

            /**
             * Logs trace message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             */
            template <typename... Args>
            static void trace(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (!m_instance) Logger::configure(karabo::data::Hash());
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->trace(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->trace(fmt, std::forward<Args>(args)...);
            }

            /**
             * Logs debug message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             * @param ... following by args
             */
            template <typename... Args>
            static void debug(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (!m_instance) Logger::configure(karabo::data::Hash());
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->debug(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->debug(fmt, std::forward<Args>(args)...);
            }

            /**
             * Logs info message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             */
            template <typename... Args>
            static void info(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (!m_instance) Logger::configure(karabo::data::Hash());
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->info(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->info(fmt, std::forward<Args>(args)...);
            }

            /**
             * Logs warn message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             */
            template <typename... Args>
            static void warn(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (!m_instance) Logger::configure(karabo::data::Hash());
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->warn(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->warn(fmt, std::forward<Args>(args)...);
            }

            /**
             * Logs error message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             */
            template <typename... Args>
            static void error(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (!m_instance) Logger::configure(karabo::data::Hash());
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->error(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->error(fmt, std::forward<Args>(args)...);
            }

            /**
             * Logs critical message using specified logger
             * @param name of registered logger
             * @param fmt - format string followed by optional args
             */
            template <typename... Args>
            static void critical(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&... args) {
                if (name == KARABO_AUDIT_LOGGER) {
                    if (!spdlog::get(KARABO_AUDIT_LOGGER)) {
                        throw KARABO_PARAMETER_EXCEPTION(std::string("Do not use reserved logger name: ") +
                                                         KARABO_AUDIT_LOGGER);
                    }
                    if (!Logger::m_audit) {
                        throw KARABO_PARAMETER_EXCEPTION("Activate audit channel 'useAuditFile()' first!");
                    }
                    Logger::m_audit->critical(fmt, std::forward<Args>(args)...);
                    Logger::m_audit->flush();
                    return;
                }
                auto logger = karabo::log::details::getLogger(name);
                if (logger) logger->critical(fmt, std::forward<Args>(args)...);
            }

            /**
             * Allows to set the level filter for specified logger or globally (empty 'logger')
             * @param level TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL
             * @param logger The category to apply the filter to (empty string reflects default logger)
             */
            static void setLevel(const std::string& level, const std::string& logger = "");

            /**
             * Retrieve the currently enabled level for the given logger name or global level
             * @param logger The logger name (empty string reflects default logger)
             */
            static const std::string getLevel(const std::string& logger = "");

            static void setPattern(const std::string& pattern, const std::string& logger = "");

            /**
             * Get (log) messages in internal ringbuffer
             */
            static std::vector<karabo::data::Hash> getCachedContent(size_t lim);

            static std::shared_ptr<spdlog::logger> getCategory(const std::string& name);

           private:
            static std::shared_ptr<spdlog::logger> create_new_default();
            static std::shared_ptr<spdlog::sinks::sink> _useConsole();
            static std::shared_ptr<spdlog::sinks::sink> _useCache();
            static std::shared_ptr<spdlog::sinks::sink> _useFile();
            static void applyRotationRulesFor(const std::filesystem::path& fname, std::size_t maxFiles);

           private:
            Logger() = default;
            static std::shared_ptr<Logger> m_instance;
            static karabo::data::Hash m_config;
            static std::shared_ptr<spdlog::logger> m_audit;
            static std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> m_ring;
            static int m_sinks;
            static std::map<std::string, std::vector<std::shared_ptr<spdlog::sinks::sink>>> m_usemap;
            static std::mutex m_globalLoggerMutex;
        };

    } // namespace log
} // namespace karabo
#endif

// Macros for logging using stream style (deprecated! 20% slower than function style)
// for example, KARABO_LOG_FRAMEWORK_INFO << "Hello, " << "World!\n";
// or, KARABO_LOG_FRAMEWORK_DEBUG_C("LOCAL_DAQ/DM/DA01") << "Hello, " << "Data Aggregator!";

// Convenient logging

#ifdef KARABO_ENABLE_TRACE_LOG

#define KARABO_LOG_FRAMEWORK_TRACE KARABO_LOG_FRAMEWORK_DEBUG

#define KARABO_LOG_FRAMEWORK_TRACE_C(category) KARABO_LOG_FRAMEWORK_DEBUG_C(category)

#define KARABO_LOG_FRAMEWORK_TRACE_CF KARABO_LOG_FRAMEWORK_DEBUG_C(Self::classInfo().getLogCategory() + "." + __func__)

#else

#define KARABO_LOG_FRAMEWORK_TRACE \
    if (1)                         \
        ;                          \
    else std::cerr

#define KARABO_LOG_FRAMEWORK_TRACE_C(category) \
    if (1)                                     \
        ;                                      \
    else std::cerr
#define KARABO_LOG_FRAMEWORK_TRACE_CF \
    if (1)                            \
        ;                             \
    else std::cerr

#endif

#define KARABO_LOG_FRAMEWORK_DEBUG karabo::log::LoggerStream(Self::classInfo().getLogCategory(), spdlog::level::debug)
#define KARABO_LOG_FRAMEWORK_INFO karabo::log::LoggerStream(Self::classInfo().getLogCategory(), spdlog::level::info)
#define KARABO_LOG_FRAMEWORK_WARN karabo::log::LoggerStream(Self::classInfo().getLogCategory(), spdlog::level::warn)
#define KARABO_LOG_FRAMEWORK_ERROR karabo::log::LoggerStream(Self::classInfo().getLogCategory(), spdlog::level::err)


#define KARABO_LOG_FRAMEWORK_DEBUG_C(id) karabo::log::LoggerStream(id, spdlog::level::debug)
#define KARABO_LOG_FRAMEWORK_INFO_C(id) karabo::log::LoggerStream(id, spdlog::level::info)
#define KARABO_LOG_FRAMEWORK_WARN_C(id) karabo::log::LoggerStream(id, spdlog::level::warn)
#define KARABO_LOG_FRAMEWORK_ERROR_C(id) karabo::log::LoggerStream(id, spdlog::level::err)

// Macros for logging using function (fmtlib) style (recommended!)
// For example, KARABO_LOGGING_DEBUG("*** Hello, {}!\n", "World");
#define KARABO_LOGGING_TRACE(...) karabo::log::Logger::trace(Self::classInfo().getLogCategory(), __VA_ARGS__)
#define KARABO_LOGGING_DEBUG(...) karabo::log::Logger::debug(Self::classInfo().getLogCategory(), __VA_ARGS__)
#define KARABO_LOGGING_INFO(...) karabo::log::Logger::info(Self::classInfo().getLogCategory(), __VA_ARGS__)
#define KARABO_LOGGING_WARN(...) karabo::log::Logger::warn(Self::classInfo().getLogCategory(), __VA_ARGS__)
#define KARABO_LOGGING_ERROR(...) karabo::log::Logger::error(Self::classInfo().getLogCategory(), __VA_ARGS__)
#define KARABO_LOGGING_FATAL(...) karabo::log::Logger::critical(Self::classInfo().getLogCategory(), __VA_ARGS__)

#define KARABO_LOGGING_TRACE_C(id, ...) karabo::log::Logger::trace(id, __VA_ARGS__)
#define KARABO_LOGGING_DEBUG_C(id, ...) karabo::log::Logger::debug(id, __VA_ARGS__)
#define KARABO_LOGGING_INFO_C(id, ...) karabo::log::Logger::info(id, __VA_ARGS__)
#define KARABO_LOGGING_WARN_C(id, ...) karabo::log::Logger::warn(id, __VA_ARGS__)
#define KARABO_LOGGING_ERROR_C(id, ...) karabo::log::Logger::error(id, __VA_ARGS__)
#define KARABO_LOGGING_FATAL_C(id, ...) karabo::log::Logger::critical(id, __VA_ARGS__)

#define KARABO_AUDIT_TRACE(...) karabo::log::Logger::trace(KARABO_AUDIT_LOGGER, __VA_ARGS__)
#define KARABO_AUDIT_DEBUG(...) karabo::log::Logger::debug(KARABO_AUDIT_LOGGER, __VA_ARGS__)
#define KARABO_AUDIT_INFO(...) karabo::log::Logger::info(KARABO_AUDIT_LOGGER, __VA_ARGS__)
#define KARABO_AUDIT_WARN(...) karabo::log::Logger::warn(KARABO_AUDIT_LOGGER, __VA_ARGS__)
#define KARABO_AUDIT_ERROR(...) karabo::log::Logger::error(KARABO_AUDIT_LOGGER, __VA_ARGS__)
#define KARABO_AUDIT_FATAL(...) karabo::log::Logger::critical(KARABO_AUDIT_LOGGER, __VA_ARGS__)
