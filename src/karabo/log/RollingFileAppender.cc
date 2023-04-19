/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "RollingFileAppender.hh"

#include <krb_log4cpp/PatternLayout.hh>
#include <krb_log4cpp/RollingFileAppender.hh>

#include "karabo/util/NodeElement.hh"
#include "karabo/util/PathElement.hh"
#include "karabo/util/SimpleElement.hh"


using namespace std;
using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::log::RollingFileAppender)

namespace karabo {
    namespace log {

        void RollingFileAppender::expectedParameters(Schema& s) {
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
                  .defaultValue("karabo.log")
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

            STRING_ELEMENT(s)
                  .key("threshold")
                  .description(
                        "The Appender will not appended log events with a priority lower than the threshold.\
                                  Use Priority::NOTSET to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("NOTSET")
                  .commit();
        }


        RollingFileAppender::RollingFileAppender(const Hash& input) {
            // The raw pointers will be handed to a log4cpp category object (functionality of Logger class)
            // Log4cpp will take ownership and deal with memory management
            krb_log4cpp::PatternLayout* layout = new krb_log4cpp::PatternLayout();
            layout->setConversionPattern(input.get<string>("pattern"));

            m_appender = new krb_log4cpp::RollingFileAppender("file", input.get<string>("filename"),
                                                              input.get<unsigned int>("maxFileSize"),
                                                              input.get<unsigned int>("maxBackupIndex"));
            m_appender->setLayout(layout);
            m_appender->setThreshold(krb_log4cpp::Priority::getPriorityValue(input.get<string>("threshold")));
        }


        krb_log4cpp::Appender* RollingFileAppender::getAppender() {
            return m_appender;
        }

    } // namespace log
} // namespace karabo
