/*
 * $Id: OstreamAppenderConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   OstreamAppenderConfigurator.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August 26, 2010, 1:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <string>
#include "OstreamAppenderConfigurator.hh"
#include "krb_log4cpp/OstreamAppender.hh"
#include <karabo/util/SimpleElement.hh>

using namespace std;
using namespace karabo::util;
using namespace krb_log4cpp;


namespace karabo {
    namespace log {


        KARABO_REGISTER_FOR_CONFIGURATION(AppenderConfigurator, OstreamAppenderConfigurator)

        void OstreamAppenderConfigurator::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .description("Output stream")
                    .key("output")
                    .displayedName("OutputStream")
                    .options("STDERR,STDOUT")
                    .assignmentOptional().defaultValue("STDERR")
                    .commit();
        }


        OstreamAppenderConfigurator::OstreamAppenderConfigurator(const Hash& input) : AppenderConfigurator(input) {
            m_out = input.get<string > ("output");
        }


        krb_log4cpp::Appender* OstreamAppenderConfigurator::create() {
            if (m_out == "STDERR") {
                return new krb_log4cpp::OstreamAppender(getName(), &std::cerr);
            }
            if (m_out == "STDOUT") {
                return new krb_log4cpp::OstreamAppender(getName(), &std::cout);
            }
            return new krb_log4cpp::OstreamAppender(getName(), &std::cerr);
        }
    }
}
