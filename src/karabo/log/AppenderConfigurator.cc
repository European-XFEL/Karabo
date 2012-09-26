/*
 * $Id: AppenderConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   AppenderConfigurator.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on September 20, 2010, 10:33 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "AppenderConfigurator.hh"
#include "log4cpp/Layout.hh"
#include "LayoutConfigurator.hh"
using namespace std;
using namespace karabo::util;
using namespace log4cpp;


namespace karabo {
  namespace log {

    void AppenderConfigurator::expectedParameters(Schema& expected) {


      STRING_ELEMENT(expected)
              .key("name")
              .displayedName("AppenderName")
              .description("Appender name")
              .assignmentOptional().defaultValue("default")
              .commit();


      STRING_ELEMENT(expected)
              .description("Set the threshold for the appender")
              .key("threshold")
              .displayedName("Threshold")
              .options("DEBUG INFO WARN ERROR NOTSET")
              .assignmentOptional() .defaultValue("NOTSET")
              .commit();


      CHOICE_ELEMENT<LayoutConfigurator > (expected)
              .key("layout")
              .displayedName("Layout")
              .description("Configures layout")
              .assignmentOptional().defaultValue("Simple")
              .commit();


    }

    void AppenderConfigurator::configure(const Hash& input) {
      configureName(input);
      configureThreshold(input);
      configureLayout(input);
    }

    void AppenderConfigurator::configureName(const Hash& input) {
      m_appenderName = input.get<string > ("name");
    }

    void AppenderConfigurator::configureThreshold(const Hash& input) {
      string threshold = input.get<string > ("threshold");
      m_threshold = Priority::getPriorityValue(threshold);

    }

    void AppenderConfigurator::configureLayout(const Hash& input) {
      LayoutConfigurator::Pointer layoutConfig = LayoutConfigurator::createChoice("layout", input);
      m_layout = layoutConfig->create();

    }

    Appender* AppenderConfigurator::getConfigured() {
      Appender* appender = create();
      assert(appender != 0);
      appender->setLayout(m_layout);
      if (m_threshold != log4cpp::Priority::NOTSET) {
        appender->setThreshold(m_threshold);
      }
      return appender;
    }


  }
}
