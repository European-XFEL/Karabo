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
#include "log4cpp/OstreamAppender.hh"

using namespace std;
using namespace exfel::util;
using namespace log4cpp;


namespace exfel {
  namespace log {

    OstreamAppenderConfigurator::OstreamAppenderConfigurator() {
    }

    OstreamAppenderConfigurator::~OstreamAppenderConfigurator() {
    }

    log4cpp::Appender* OstreamAppenderConfigurator::create() {
      if (m_out == "STDERR") {
        return new log4cpp::OstreamAppender(getName(), &std::cerr);
      }
      if (m_out == "STDOUT") {
        return new log4cpp::OstreamAppender(getName(), &std::cout);
      }
      return new log4cpp::OstreamAppender(getName(), &std::cerr);
    }

    void OstreamAppenderConfigurator::expectedParameters(Schema& expected) {


      STRING_ELEMENT(expected)
              .description("Output stream")
              .key("output")
              .displayedName("OutputStream")
              .options("STDERR,STDOUT")
              .assignmentOptional().defaultValue("STDERR")
              .commit();
    }

    void OstreamAppenderConfigurator::configure(const Hash& input) {
      m_out = input.get<string > ("output");
    }

    EXFEL_REGISTER_FACTORY_CC(AppenderConfigurator, OstreamAppenderConfigurator)

  }
}
