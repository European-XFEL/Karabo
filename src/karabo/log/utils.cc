#include "utils.hh"

#include <spdlog/common.h>
#include <spdlog/details/log_msg_buffer.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Logger.hh"
#include "karabo/data/types/StringTools.hh"

namespace karabo {
    namespace log {
        namespace details {


            std::shared_ptr<spdlog::logger> getLogger(const std::string& name) {
                std::lock_guard<std::mutex> lock(Logger::m_globalLoggerMutex);
                std::shared_ptr<spdlog::logger> mylog = spdlog::get(name);
                if (mylog) return mylog;

                std::set<std::string> names = {};
                // scan logger registry to collect all logger names ...
                spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { names.insert(l->name()); });
                if (names.empty()) {
                    // registry empty ... perhaps after 'shutdown' command...
                    auto deflog = spdlog::default_logger();
                    if (!deflog) deflog = Logger::create_new_default();
                    if (name.empty()) return deflog;
                    mylog = deflog->clone(name);
                    if (!spdlog::get(name)) spdlog::register_logger(mylog);
                    return mylog;
                }
                // Go through the sorted names from end to start to find the most specific category for cloning...
                for (auto it = names.crbegin(); it != names.crend(); ++it) {
                    const std::string& category = *it;
                    if (category.empty() || name.find(category) == 0) {
                        // Clone new logger from found logger with its settings including sinks, pattern, level
                        auto srclog = spdlog::get(category);
                        if (!srclog) {
                            srclog = spdlog::default_logger();
                            if (!srclog) srclog = Logger::create_new_default();
                        }
                        mylog = srclog->clone(name); // Clone 'name' logger from 'category' one ...
                        // Register it
                        if (mylog) spdlog::register_logger(mylog);
                        break;
                    }
                }
                return mylog;
            }

        } // namespace details
    } // namespace log
} // namespace karabo
