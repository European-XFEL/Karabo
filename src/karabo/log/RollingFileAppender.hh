/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOG_ROLLINGFILEAPPENDER_HH
#define	KARABO_LOG_ROLLINGFILEAPPENDER_HH

#include "karabo/util/Configurator.hh"
#include <boost/filesystem.hpp>

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

            krb_log4cpp::Appender* getAppender();

        private:

            krb_log4cpp::Appender* m_appender;

        };
    }
}

#endif
