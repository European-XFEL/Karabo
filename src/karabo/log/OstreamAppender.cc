/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "OstreamAppender.hh"

#include <iostream>
#include <karabo/util/SimpleElement.hh>
#include <krb_log4cpp/OstreamAppender.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <string>


using namespace std;
using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::log::OstreamAppender)

namespace karabo {
    namespace log {

        void OstreamAppender::expectedParameters(Schema& s) {
            STRING_ELEMENT(s)
                  .key("output")
                  .description("Output Stream")
                  .displayedName("OutputStream")
                  .options(std::vector<std::string>({"STDERR", "STDOUT"}))
                  .assignmentOptional()
                  .defaultValue("STDERR")
                  .commit();

            STRING_ELEMENT(s)
                  .key("pattern")
                  .description("Formatting pattern for the logstream")
                  .displayedName("Pattern")
                  .assignmentOptional()
                  .defaultValue("%d{%Y-%m-%dT%H:%M:%S.%l} %p  %c  : %m%n")
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


        OstreamAppender::OstreamAppender(const Hash& config) {
            // The raw pointers will be handed to a log4cpp category object (functionality of Logger class)
            // Log4cpp will take ownership and deal with memory management (proper delete)
            krb_log4cpp::PatternLayout* layout = new krb_log4cpp::PatternLayout();
            layout->setConversionPattern(config.get<string>("pattern"));

            if (config.get<string>("output") == "STDOUT") {
                m_appender = new krb_log4cpp::OstreamAppender("console", &cout);
            } else {
                m_appender = new krb_log4cpp::OstreamAppender("console", &cerr);
            }

            m_appender->setLayout(layout);
            m_appender->setThreshold(krb_log4cpp::Priority::getPriorityValue(config.get<string>("threshold")));
        }


        krb_log4cpp::Appender* OstreamAppender::getAppender() {
            return m_appender;
        }
    } // namespace log
} // namespace karabo
