#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <sstream>

#include "utils.hh"


namespace karabo {
    namespace log {

        class LoggerStream {
           public:
            /**
             * Create stream using registered logger object and message level
             */
            LoggerStream(const std::string& name, spdlog::level::level_enum lvl)
                : m_name(name), m_oss(), m_level(lvl) {}

            ~LoggerStream();

            template <typename T>
            LoggerStream& operator<<(const T& value) {
                m_oss << value;
                return *this;
            }

           private:
            std::string m_name;
            std::ostringstream m_oss;
            spdlog::level::level_enum m_level;
        };

    } // namespace log
} // namespace karabo
