#ifndef KARABO_LOG_UTILS_HH
#define KARABO_LOG_UTILS_HH

#include <spdlog/spdlog.h>

namespace karabo {
    namespace log {
        namespace details {

            std::shared_ptr<spdlog::logger> getLogger(const std::string& name);

        } // namespace details
    } // namespace log
} // namespace karabo

#endif /*KARABO_LOG_UTILS_HH*/
