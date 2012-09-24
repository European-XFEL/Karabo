/*
 * $Id: OstreamAppenderConfigurator.hh 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   OstreamAppenderConfigurator.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on August 26, 2010, 1:12 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH

#include "AppenderConfigurator.hh"
#include <karabo/util/Factory.hh>
#include <log4cpp/Appender.hh>
#include <string>
/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package log
   */
  namespace log {

    class OstreamAppenderConfigurator : public AppenderConfigurator {
    public:
      EXFEL_CLASSINFO(OstreamAppenderConfigurator, "Ostream", "1.0")

      OstreamAppenderConfigurator();

      virtual ~OstreamAppenderConfigurator();

      log4cpp::Appender* create();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    private:
      std::string m_out;


    };

  }
}

#endif	/* EXFEL_LOGCONFIG_OSTREAMAPPENDERCONFIGURATOR_HH */
