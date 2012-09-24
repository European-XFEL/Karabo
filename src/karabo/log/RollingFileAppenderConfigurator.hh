/*
 * $Id: RollingFileAppenderConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   RollingFileAppenderConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH

#include "FileAppenderConfigurator.hh"
#include <karabo/util/Factory.hh>
#include <boost/filesystem.hpp>

namespace log4cpp {
  class Appender;
}

/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package log
   */
  namespace log {

    class RollingFileAppenderConfigurator : public FileAppenderConfigurator {

      unsigned int m_maxFileSize;
      unsigned short m_maxBackupIndex;
      

    public:

      EXFEL_CLASSINFO(RollingFileAppenderConfigurator, "RollingFile", "1.0")

      RollingFileAppenderConfigurator();
      virtual ~RollingFileAppenderConfigurator();


      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

      log4cpp::Appender* create();
      
    private:
      void configureMaxSize(const exfel::util::Hash& input);
      void configureMaxBackupIndex(const exfel::util::Hash& input);
      
    };

  }
}

#endif	/* EXFEL_LOGCONFIG_ROLLINGFILEAPPENDERCONFIGURATOR_HH */
