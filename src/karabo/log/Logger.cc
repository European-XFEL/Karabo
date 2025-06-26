/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "Logger.hh"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>

#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/types/StringTools.hh"


using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace karabo::data;
using namespace std;


namespace karabo {
    namespace log {

        // Static initialization of LogStreamRegistry
        std::shared_ptr<Logger> Logger::m_instance = nullptr;
        karabo::data::Hash Logger::m_config = karabo::data::Hash();
        std::shared_ptr<spdlog::logger> Logger::m_audit = nullptr;
        std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> Logger::m_ring = nullptr;
        int Logger::m_sinks = 0;
        std::map<std::string, std::vector<std::shared_ptr<spdlog::sinks::sink>>> Logger::m_usemap;
        std::mutex Logger::m_globalLoggerMutex;


        void Logger::expectedParameters(Schema& s) {
            // Take care to keep this level in sync with "Logger.level" of the Python karabo/bound/device.py
            STRING_ELEMENT(s)
                  .key("level")
                  .displayedName("Level")
                  .description("The default log level")
                  .options("DEBUG INFO WARN ERROR FATAL")
                  .assignmentOptional()
                  .defaultValue("INFO")
                  .commit();

            STRING_ELEMENT(s)
                  .key("pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %n : %v")
                  .commit();

            NODE_ELEMENT(s).key("console").commit();

            STRING_ELEMENT(s)
                  .key("console.output")
                  .description("Output Stream")
                  .displayedName("OutputStream")
                  .options(std::vector<std::string>({"STDERR", "STDOUT"}))
                  .assignmentOptional()
                  .defaultValue("STDERR")
                  .commit();

            STRING_ELEMENT(s)
                  .key("console.pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %n : %v")
                  .commit();

            STRING_ELEMENT(s)
                  .key("console.threshold")
                  .description(
                        "The Logger will not log messages with a level lower than defined here.\
                                  Use Level::NOTSET to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("NOTSET")
                  .commit();

            NODE_ELEMENT(s).key("file").commit();

            STRING_ELEMENT(s)
                  .key("file.filename")
                  .description("Filename")
                  .displayedName("Filename")
                  .assignmentOptional()
                  .defaultValue("karabo.log")
                  .commit();

            UINT32_ELEMENT(s)
                  .key("file.maxFileSize")
                  .description("Maximum file size")
                  .displayedName("MaxFileSize")
                  .unit(Unit::BYTE)
                  .assignmentOptional()
                  .defaultValue(10 * 1024 * 1024)
                  .commit();

            UINT32_ELEMENT(s)
                  .key("file.maxBackupIndex")
                  .description("Maximum backup index (rolling file index)")
                  .displayedName("MaxBackupIndex")
                  .assignmentOptional()
                  .defaultValue(10)
                  .commit();

            STRING_ELEMENT(s)
                  .key("file.pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %n : %v")
                  .commit();

            STRING_ELEMENT(s)
                  .key("file.threshold")
                  .description(
                        "The sink will not appended log message with a level lower than the threshold.\
                                  Use Level::NOTSET or OFF to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("NOTSET")
                  .commit();


            NODE_ELEMENT(s).key("cache").commit();

            STRING_ELEMENT(s)
                  .key("cache.logger")
                  .description("Name identifying the logger")
                  .displayedName("Logger name")
                  .assignmentOptional()
                  .defaultValue("") // default logger
                  .commit();

            UINT32_ELEMENT(s)
                  .key("cache.maxNumMessages")
                  .displayedName("Max. Num. Messages")
                  .description("Maximum number of messages in the cache")
                  .assignmentOptional()
                  .defaultValue(100u)
                  .maxInc(1000u)
                  .commit();

            STRING_ELEMENT(s)
                  .key("cache.pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %n : %v")
                  .commit();

            STRING_ELEMENT(s)
                  .key("cache.threshold")
                  .description(
                        "The Logger will not log messages with a level lower than defined here.\
                                  Use Level::NOTSET to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("INFO")
                  .commit();

            NODE_ELEMENT(s).key("audit").commit();

            STRING_ELEMENT(s)
                  .key("audit.logger")
                  .description("Audit logger name")
                  .displayedName("Audit logger")
                  .assignmentOptional()
                  .defaultValue(KARABO_AUDIT_LOGGER)
                  .commit();

            STRING_ELEMENT(s)
                  .key("audit.filename")
                  .description("Filename of audit file data")
                  .displayedName("Filename")
                  .assignmentOptional()
                  .defaultValue("audit.log")
                  .commit();

            UINT32_ELEMENT(s)
                  .description("Hour for file rotation")
                  .key("audit.hour")
                  .displayedName("Hour")
                  .assignmentOptional()
                  .defaultValue(2)
                  .minInc(0)
                  .maxInc(23)
                  .commit();

            UINT32_ELEMENT(s)
                  .key("audit.minute")
                  .description("Minute for file rotation")
                  .displayedName("Minute")
                  .assignmentOptional()
                  .defaultValue(30)
                  .minInc(0)
                  .maxInc(59)
                  .commit();

            UINT16_ELEMENT(s)
                  .key("audit.maxFiles")
                  .description("Maximum number of files in the files ring; 0 - no limits")
                  .displayedName("MaxBackups")
                  .assignmentOptional()
                  .defaultValue(14)
                  .commit();

            STRING_ELEMENT(s)
                  .key("audit.pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%Y-%m-%d %H:%M:%S.%e [%l] AUDIT : %v")
                  .commit();

            STRING_ELEMENT(s)
                  .key("audit.threshold")
                  .description(
                        "The sink will not appended log message with a level lower than the threshold.\
                                  Use Level::NOTSET or OFF to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("INFO")
                  .commit();
        }


        void Logger::configure(const karabo::data::Hash& config) {
            bool startFlushThread = false;
            if (!Logger::m_instance) {
                // 'spdlog' default settings provides default_logger with console sink
                // In karabo console (ostream) sink should be activated by useConsole
                spdlog::default_logger()->sinks().clear();
                Logger::m_instance = std::shared_ptr<Logger>(new Logger);
                startFlushThread = true;
            }
            // Clear m_config in case a call to configure happened before
            m_config.clear();
            // Validate given configuration
            Schema schema;
            expectedParameters(schema);
            Validator validator; // Default validation
            std::pair<bool, std::string> ret = validator.validate(schema, config, m_config);
            if (!ret.first) { // Validation failed
                throw KARABO_PARAMETER_EXCEPTION("Logger configuration failed. \n" + ret.second);
            }

            m_audit = nullptr;
            if (startFlushThread) {
                setPattern(m_config.get<std::string>("pattern"));
                setLevel("OFF");
                spdlog::flush_every(3s);
                startFlushThread = false;
            }
        }


        LoggerStream::~LoggerStream() {
            if (!Logger::m_instance) Logger::configure(Hash());
            auto l = details::getLogger(m_name);
            if (!l) return;
            l->log(m_level, m_oss.str().c_str());
        }


        std::shared_ptr<spdlog::sinks::sink> Logger::_useConsole() {
            std::shared_ptr<spdlog::sinks::sink> sink = nullptr;
            if (m_config.get<std::string>("console.output") == "STDOUT") {
                sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            } else {
                sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
            }
            sink->set_pattern(m_config.get<std::string>("console.pattern"));
            auto val = m_config.get<std::string>("level");
            toLower(val);
            sink->set_level(spdlog::level::from_str(val));
            return sink;
        }


        std::shared_ptr<spdlog::sinks::sink> Logger::_useFile() {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                  m_config.get<std::string>("file.filename"), m_config.get<unsigned int>("file.maxFileSize"),
                  m_config.get<unsigned int>("file.maxBackupIndex"));
            sink->set_pattern(m_config.get<std::string>("file.pattern"));
            auto val = m_config.get<std::string>("level");
            toLower(val);
            sink->set_level(spdlog::level::from_str(val));
            return sink;
        }


        std::shared_ptr<spdlog::sinks::sink> Logger::_useCache() {
            auto sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(
                  m_config.get<unsigned int>("cache.maxNumMessages"));
            sink->set_pattern(m_config.get<std::string>("cache.pattern"));
            auto val = m_config.get<std::string>("level");
            toLower(val);
            sink->set_level(spdlog::level::from_str(val));
            Logger::m_ring = sink;
            return sink;
        }


        void Logger::useConsole(const std::string& name, bool inheritSinks) {
            if (!m_instance) configure(Hash());
            auto sink = _useConsole();
            std::shared_ptr<spdlog::logger> logger = nullptr;
            if (name.empty()) logger = spdlog::default_logger();
            else logger = spdlog::get(name);
            if (!logger) logger = details::getLogger(name);
            if (!inheritSinks) logger->sinks().clear();
            logger->sinks().push_back(sink);
            setLevel(m_config.get<std::string>("level"), name);
        }


        void Logger::useFile(const std::string& name, bool inheritSinks) {
            if (!m_instance) configure(Hash());
            auto sink = _useFile();
            std::shared_ptr<spdlog::logger> logger = nullptr;
            if (name.empty()) logger = spdlog::default_logger();
            else logger = spdlog::get(name);
            if (!logger) logger = details::getLogger(name);
            if (!inheritSinks) logger->sinks().clear();
            logger->sinks().push_back(sink);
            setLevel(m_config.get<std::string>("level"), name);
        }


        void Logger::useCache(const std::string& name, bool inheritSinks) {
            if (!m_instance) configure(Hash());
            auto sink = _useCache();
            std::shared_ptr<spdlog::logger> logger = nullptr;
            if (name.empty()) logger = spdlog::default_logger();
            else logger = spdlog::get(name);
            if (!logger) logger = details::getLogger(name);
            if (!inheritSinks) logger->sinks().clear();
            logger->sinks().push_back(sink);
            setLevel(m_config.get<std::string>("level"), name);
        }


        void Logger::useAuditFile(const std::string& name) {
            if (!m_instance) configure(Hash());
            if (name.empty()) {
                throw KARABO_PARAMETER_EXCEPTION("Audit file logger name should not be empty!");
            }
            if (!m_config.has("logger")) {
                m_config.set("logger", name);
            } else if (m_config.get<std::string>("logger") != name) {
                throw KARABO_PARAMETER_EXCEPTION("Audit logger name mismatch in argument and config");
            }
            std::filesystem::path fname(m_config.get<std::string>("audit.filename"));
            uint16_t maxFiles = m_config.get<uint16_t>("audit.maxFiles");
            uint32_t audit_hour = m_config.get<uint32_t>("audit.hour");
            uint32_t audit_min = m_config.get<uint32_t>("audit.minute");

            // Clean archive of log-files using 'maxFiles' filter
            applyRotationRulesFor(fname, maxFiles);
            auto log = spdlog::daily_logger_mt(name, fname.string(), audit_hour, audit_min, false, maxFiles);

            log->set_pattern(m_config.get<std::string>("audit.pattern"));
            auto val = m_config.get<std::string>("audit.threshold");
            toLower(val);
            log->set_level(spdlog::level::from_str(val));
            setLevel(m_config.get<std::string>("level"), name);
            m_audit = log;
        }


        void Logger::applyRotationRulesFor(const std::filesystem::path& fpath, std::size_t maxFiles) {
            namespace fs = std::filesystem;
            // get directory where log file located ...
            fs::path dir = fpath.parent_path();
            // get file stem: name without dir path and extension
            auto stem = fpath.stem().string();
            std::map<std::size_t, fs::path> tmpmap;
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                auto p = entry.path();
                // filter entries where 'filename' starts with 'stem'
                std::size_t found = p.filename().string().find(stem);
                if (found == std::string::npos) continue;
                if (found != 0) continue;
                // get last file modified time since epoch ...
                auto epoch = entry.last_write_time().time_since_epoch();
                // convert to seconds since epoch
                std::size_t secs = duration_cast<seconds>(epoch).count();
                // use this seconds for sorting in the map
                tmpmap.emplace(secs, p);
            }
            size_t size = tmpmap.size();
            if (size > maxFiles) {
                auto removeCount = size - maxFiles;
                // remove the oldest 'removeCount' paths...
                for (auto it = tmpmap.begin(); it != tmpmap.end(); ++it) {
                    fs::remove(it->second);
                    if (--removeCount == 0) break;
                }
            }
        }


        void Logger::setPattern(const std::string& pattern, const std::string& name) {
            if (name.empty()) {
                // apply new pattern to all existing loggers
                spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->set_pattern(pattern); });
                return;
            }
            if (!spdlog::get(name)) {
                throw KARABO_PARAMETER_EXCEPTION("No logger is registered with name : " + name);
            }
            karabo::log::details::getLogger(name)->set_pattern(pattern);
        }


        void Logger::setLevel(const std::string& level, const std::string& category) {
            std::string new_level = level;
            toLower(new_level);                            // update 'new_level' in place
            auto lvl = spdlog::level::from_str(new_level); // convert to level::level_enum
            // apply new level to all known loggers
            spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
                auto name = l->name();
                if (category.empty()) {
                    l->set_level(lvl);
                    for (auto& s : l->sinks()) {
                        s->set_level(lvl);
                    }
                } else if (name.empty()) {
                    return;
                } else {
                    size_t pos = name.find(category);
                    if (pos != 0) return;
                    l->set_level(lvl);
                    for (auto& s : l->sinks()) {
                        s->set_level(lvl);
                    }
                }
            });
        }


        const std::string Logger::getLevel(const std::string& name) {
            using namespace spdlog;
            level::level_enum level_num = level::off;
            if (name.empty()) {
                level_num = get_level();
            } else if (!get(name)) {
                throw KARABO_PARAMETER_EXCEPTION("No registered logger found with the name : " + name);
            } else {
                level_num = get(name)->level();
            }

            switch (level_num) {
                case level::trace:
                    return "TRACE";
                case level::debug:
                    return "DEBUG";
                case level::info:
                    return "INFO";
                case level::warn:
                    return "WARN";
                case level::err:
                    return "ERROR";
                case level::critical:
                    return "FATAL";
                default:
                    break;
            }
            return "OFF";
        }

        std::vector<karabo::data::Hash> Logger::getCachedContent(size_t lim) {
            std::vector<Hash> ret;
            if (!Logger::m_ring) return ret;
            std::vector<std::string> lines = Logger::m_ring->last_formatted(lim);
            for (const auto& line : lines) {
                size_t pos = line.find_first_of(":", 25);
                string prefix(line.begin(), line.begin() + pos);
                string message(line.begin() + pos + 2, line.end());
                trim(message);
                std::vector<std::string> tmp;
                boost::split(tmp, prefix, boost::is_any_of(" \n\t"), boost::token_compress_on);
                assert(tmp.size() >= 4);
                std::string type = "UNKNOWN";
                if (tmp[2] == "[trace]") type = "TRACE";
                else if (tmp[2] == "[debug]") type = "DEBUG";
                else if (tmp[2] == "[info]") type = "INFO";
                else if (tmp[2] == "[warning]") type = "WARN";
                else if (tmp[2] == "[error]") type = "ERROR";
                else if (tmp[2] == "[critical]") type = "FATAL";
                std::string ts = tmp[0];
                ts += " ";
                ts += tmp[1];
                Hash entry("timestamp", ts, "type", type, "category", tmp[3], "message", message);
                ret.push_back(entry);
            }
            return ret;
        }


        std::shared_ptr<spdlog::logger> Logger::getCategory(const std::string& name) {
            if (name.empty()) return spdlog::default_logger();
            auto logger = spdlog::get(name);
            if (logger) return logger;
            return details::getLogger(name); // inherits settings (sinks, levels, ...)
        }


        std::shared_ptr<spdlog::logger> Logger::create_new_default() {
            auto logger = spdlog::get("");
            if (!logger) {
                logger = std::make_shared<spdlog::logger>("");
                spdlog::initialize_logger(logger);
            }
            spdlog::set_default_logger(logger);
            return logger;
        }


        void Logger::reset() {
            {
                std::lock_guard<std::mutex> lock(m_globalLoggerMutex);
                spdlog::shutdown();
                // Logger::m_instance = nullptr;
                Logger::m_sinks = 0;
                Logger::m_ring = nullptr;
                Logger::m_audit = nullptr;
                // restore default logger
                Logger::create_new_default();
            }
            setLevel("OFF");
            if (m_config.has("pattern")) setPattern(m_config.get<std::string>("pattern"));
            else setPattern("%Y:%m:%dT%H:%M:%S.%e [%^%l%$] %n : %v");
        }

    } // namespace log
} // namespace karabo
