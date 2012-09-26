/*
 * $Id: RollingFileAppenderConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   RollingFileAppenderConfigurator.cc
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "RollingFileAppenderConfigurator.hh"
#include <log4cpp/RollingFileAppender.hh>


using namespace std;
using namespace karabo::util;

namespace karabo {
  namespace log {

    RollingFileAppenderConfigurator::RollingFileAppenderConfigurator() {
    }

    RollingFileAppenderConfigurator::~RollingFileAppenderConfigurator() {
    }

    void RollingFileAppenderConfigurator::expectedParameters(Schema& expected) {

      UINT32_ELEMENT(expected)
              .description("Maximum file size, default unit MB, allowed range: (0 -1024MB)")
              .key("maxSize")
              .displayedName("MaxSize")
              .minExc(0)
              .maxInc(1024)
              .assignmentOptional().defaultValue(10)
              .commit();


      STRING_ELEMENT(expected)
              .description("File size unit")
              .key("maxSizeUnit")
              .displayedName("maxSizeUnit")
              .options("B kB MB GB")
              .assignmentOptional().defaultValue("MB")
              .commit();


      UINT16_ELEMENT(expected)
              .description("Maximum backup index")
              .key("maxBackupIndex")
              .displayedName("MaxBackupIndex")
              .minInc(1)
              .maxInc(256)
              .assignmentOptional().defaultValue(10)
              .commit();


    }

    void RollingFileAppenderConfigurator::configure(const Hash& input) {
      configureMaxSize(input);
      configureMaxBackupIndex(input);
    }

    log4cpp::Appender* RollingFileAppenderConfigurator::create() {

      return new log4cpp::RollingFileAppender(getName(), getFilename().string(),
              m_maxFileSize, m_maxBackupIndex,
              isAppendMode(), getAccessMode());
    }

    void RollingFileAppenderConfigurator::configureMaxSize(const karabo::util::Hash& input) {

      const unsigned int KILO = 1024;
      const unsigned int MEGA = 1024 * KILO;

      map<string, unsigned int> units;
      units["B"] = 1;
      units["kB"] = KILO;
      units["MB"] = MEGA;

      unsigned int unit = MEGA;
      unit = units[input.get<string > ("maxSizeUnit")];
      m_maxFileSize = input.getNumeric<unsigned int>("maxSize") * unit;

    }

    void RollingFileAppenderConfigurator::configureMaxBackupIndex(const karabo::util::Hash& input) {
      m_maxBackupIndex = input.getNumeric<unsigned short>("maxBackupIndex");
    }

    KARABO_REGISTER_FACTORY_2_CC(AppenderConfigurator, FileAppenderConfigurator, RollingFileAppenderConfigurator)

  }
}
