/*
 * $Id: AppenderConfigurator.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * File:   AppenderConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 26, 2010, 12:30 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifndef EXFEL_LOGCONFIG_APPENDERCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_APPENDERCONFIGURATOR_HH
#include <string>
#include <karabo/util/Factory.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/Priority.hh>
#include <log4cpp/Layout.hh>
#include "logdll.hh"

namespace exfel {
  namespace log {

    class AppenderConfigurator {
    public:
      EXFEL_CLASSINFO(AppenderConfigurator, "Appender", "1.0")
      EXFEL_FACTORY_BASE_CLASS
      
    private:

      std::string m_appenderName;
      log4cpp::Priority::Value m_threshold;
      log4cpp::Layout* m_layout;

    protected:

      const std::string& getName() const {
        return m_appenderName;
      }

      virtual log4cpp::Appender* create() = 0;

    public:

      AppenderConfigurator(const std::string& appenderName, log4cpp::Priority::Value threshold, log4cpp::Layout* layout) :
      m_appenderName(appenderName), m_threshold(threshold), m_layout(layout) {
      }

      AppenderConfigurator() :
      m_appenderName("noname"), m_threshold(log4cpp::Priority::NOTSET) {
      }

      virtual ~AppenderConfigurator() {
      }

      virtual log4cpp::Appender* getConfigured();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    private:
      void configureName(const exfel::util::Hash& input);
      void configureThreshold(const exfel::util::Hash& input);
      void configureLayout(const exfel::util::Hash& input);


    };
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::log::AppenderConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* EXFEL_LOGCONFIG_APPENDERCONFIGURATOR_HH */

