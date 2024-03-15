/*
 * File:   AuditFileAppender.hh
 *
 * Created on February 6, 2024, 9:20 AM
 *
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


#ifndef KARABO_LOG_AUDITFILEAPPENDER_HH
#define KARABO_LOG_AUDITFILEAPPENDER_HH

#include <boost/filesystem.hpp>

#include "karabo/log/AuditFileFilter.hh"
#include "karabo/util/Configurator.hh"

// Forward
namespace krb_log4cpp {
    class Appender;
} // namespace krb_log4cpp

namespace karabo::log {

    /**
     * @brief Helper class to configure an underlying log4cpp appender for auditing messages - who has
     * done what.
     *
     * The appender uses a new filesystem location and filename to avoid interfering with the existing
     * RollingFileAppender. Only entries containing the karabo::log::AUDIT_ENTRY_MARK (defined in
     * AuditFileFilter.hh) will be appended.
     *
     * NOTE: Do NOT use this class directly. It is indirectly involved in the static functions
     * of the Logger!!
     */
    class AuditFileAppender {
       public:
        KARABO_CLASSINFO(AuditFileAppender, "AuditFileAppender", "")

        static void expectedParameters(karabo::util::Schema& expected);

        AuditFileAppender(const karabo::util::Hash& input);
        virtual ~AuditFileAppender() {}

        krb_log4cpp::Appender* getAppender();

       private:
        krb_log4cpp::Appender* m_appender;
    };
} // namespace karabo::log

#endif
