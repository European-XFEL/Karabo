/*
 * File:   AuditFileAppender.cc
 *
 * Created on February 6, 2024, 9:30 AM
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

#include "AuditFileAppender.hh"

#include <krb_log4cpp/PatternLayout.hh>
#include <krb_log4cpp/RollingFileAppender.hh>

#include "karabo/util/NodeElement.hh"
#include "karabo/util/PathElement.hh"
#include "karabo/util/SimpleElement.hh"


using namespace std;
using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::log::AuditFileAppender)

namespace karabo {
    namespace log {

        void AuditFileAppender::expectedParameters(Schema& s) {
            STRING_ELEMENT(s)
                  .key("category")
                  .description("Category")
                  .displayedName("Category")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            PATH_ELEMENT(s)
                  .key("filename")
                  .description("Filename")
                  .displayedName("Filename")
                  .isOutputFile()
                  .assignmentOptional()
                  .defaultValue("audit.log")
                  .commit();

            UINT32_ELEMENT(s)
                  .description("Access mode")
                  .key("mode")
                  .displayedName("AccessMode")
                  .assignmentOptional()
                  .defaultValue((unsigned int)00644)
                  .commit();

            UINT32_ELEMENT(s)
                  .key("maxFileSize")
                  .description("Maximum file size")
                  .displayedName("MaxFileSize")
                  .unit(Unit::BYTE)
                  .assignmentOptional()
                  .defaultValue(10 * 1024 * 1024)
                  .commit();

            UINT32_ELEMENT(s)
                  .key("maxBackupIndex")
                  .description("Maximum backup index (rolling file index)")
                  .displayedName("MaxBackupIndex")
                  .assignmentOptional()
                  .defaultValue(10)
                  .commit();

            STRING_ELEMENT(s)
                  .key("pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%d{%Y-%m-%d %H:%M:%S.%l} %p  %c  : %m%n")
                  .commit();
        }


        AuditFileAppender::AuditFileAppender(const Hash& input) {
            // The raw pointers will be handed to a log4cpp category object (functionality of Logger class)
            // Log4cpp will take ownership and deal with memory management
            krb_log4cpp::PatternLayout* layout = new krb_log4cpp::PatternLayout();
            auto* filter = new karabo::log::AuditFileFilter();

            layout->setConversionPattern(input.get<string>("pattern"));

            const std::string& logfilePath = input.get<string>("filename");

            m_appender =
                  new krb_log4cpp::RollingFileAppender("auditfile", logfilePath, input.get<unsigned int>("maxFileSize"),
                                                       input.get<unsigned int>("maxBackupIndex"));
            m_appender->setLayout(layout);
            m_appender->setFilter(filter);
        }


        krb_log4cpp::Appender* AuditFileAppender::getAppender() {
            return m_appender;
        }

    } // namespace log
} // namespace karabo
