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


#ifndef KARABO_LOG_ROLLINGFILEAPPENDER_HH
#define KARABO_LOG_ROLLINGFILEAPPENDER_HH

#include <boost/filesystem.hpp>

#include "karabo/util/Configurator.hh"

// Forward
namespace krb_log4cpp {
    class Appender;
}

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        /**
         * Helper class to configure an underlying log4cpp appender.
         * NOTE: Do NOT use this class directly. It is indirectly involved in the static functions
         * of the Logger!!
         */
        class RollingFileAppender {
           public:
            KARABO_CLASSINFO(RollingFileAppender, "RollingFileAppender", "")

            static void expectedParameters(karabo::util::Schema& expected);

            RollingFileAppender(const karabo::util::Hash& input);
            virtual ~RollingFileAppender() {}

            krb_log4cpp::Appender* getAppender();

           private:
            krb_log4cpp::Appender* m_appender;
        };
    } // namespace log
} // namespace karabo

#endif
