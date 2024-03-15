/*
 * File:   AuditFileFilter.hh
 *
 * Created on February 6, 2024, 9:58 AM
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
#ifndef KARABO_LOG_AUDITFILEFILTER_HH
#define KARABO_LOG_AUDITFILEFILTER_HH

#include <krb_log4cpp/Filter.hh>
#include <krb_log4cpp/LoggingEvent.hh>

namespace karabo::log {

    static const std::string AUDIT_ENTRY_MARK{"[Audit] - "};

    class AuditFileFilter : public krb_log4cpp::Filter {
        krb_log4cpp::Filter::Decision _decide(const krb_log4cpp::LoggingEvent& event) override;
    };

} // namespace karabo::log


#endif