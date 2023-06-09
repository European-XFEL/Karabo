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
