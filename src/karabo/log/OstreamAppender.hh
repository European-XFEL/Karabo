/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_LOG_OSTREAMAPPENDER_HH
#define KARABO_LOG_OSTREAMAPPENDER_HH

#include <string>

#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"

// Forward
namespace krb_log4cpp {
    class Appender;
}

namespace karabo {

    namespace log {

        /**
         * Helper class to configure an underlying log4cpp appender.
         * NOTE: Do NOT use this class directly. It is indirectly involved by the static functions
         * of the Logger!!
         */
        class OstreamAppender {
           public:
            KARABO_CLASSINFO(OstreamAppender, "Ostream", "")

            static void expectedParameters(karabo::util::Schema& expected);

            OstreamAppender(const karabo::util::Hash& input);

            virtual ~OstreamAppender() {}

            krb_log4cpp::Appender* getAppender();

           private:
            krb_log4cpp::Appender* m_appender;
        };
    } // namespace log
} // namespace karabo
#endif
