/*
 * $Id: CategoryConfigurator.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * File:   CategoryConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_LOGCONFIG_CATEGORYCONFIGURATOR_HH
#define	EXFEL_LOGCONFIG_CATEGORYCONFIGURATOR_HH



#include <string>
#include <vector>
#include "log4cpp/Priority.hh"
#include "FileAppenderConfigurator.hh"
#include <karabo/util/Factory.hh>
#include "AppenderConfigurator.hh"
#include "logdll.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package log
   */
  namespace log {

    class CategoryConfigurator {
      std::string m_name;
      log4cpp::Priority::Value m_level;
      bool m_additivity;
      std::vector<AppenderConfigurator::Pointer> m_appenderConfigurators;

    public:
      EXFEL_CLASSINFO(CategoryConfigurator, "Category", "1.0")
      EXFEL_FACTORY_BASE_CLASS

      CategoryConfigurator();
      virtual ~CategoryConfigurator();

      void setup();

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);

    private:

      void configureName(const exfel::util::Hash& input);
      void configurePriority(const exfel::util::Hash& input);
      void configureAdditivity(const exfel::util::Hash& input);
      void configureAppenders(const exfel::util::Hash& input);
    };

  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::log::CategoryConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* EXFEL_LOGCONFIG_CATEGORYCONFIGURATOR_HH */
