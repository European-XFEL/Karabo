/*
 * File:   AuditFileFilter.cc
 *
 * Created on February 6, 2024, 10:10 AM
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
#include "AuditFileFilter.hh"

#include <iostream>

namespace karabo::log {
    using namespace krb_log4cpp;

    Filter::Decision AuditFileFilter::_decide(const krb_log4cpp::LoggingEvent& event) {
        if (event.message.find(karabo::log::AUDIT_ENTRY_MARK) == std::string::npos) {
            return Filter::Decision::DENY;
        }
        return Filter::Decision::ACCEPT;
    }

} // namespace karabo::log
