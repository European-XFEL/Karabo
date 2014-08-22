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

#include <krb_log4cpp/BasicLayout.hh>
#include <krb_log4cpp/SimpleLayout.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/NodeElement.hh>

#include "AppenderConfigurator.hh"

using namespace std;
using namespace karabo::util;
using namespace krb_log4cpp;

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

            CHOICE_ELEMENT(expected)
                    .key("layout")
                    .displayedName("Layout")
                    .description("Configures layout")
                    .assignmentOptional().defaultValue("Simple")
                    .commit();

            NODE_ELEMENT(expected).key("layout.Simple")
                    .description("Simple Layout")
                    .displayedName("Simple")
                    .commit();

            NODE_ELEMENT(expected).key("layout.Basic")
                    .description("Basic Layout")
                    .displayedName("Basic")
                    .commit();
            
            NODE_ELEMENT(expected).key("layout.Pattern")
                    .description("Allows to define a pattern for the log string")
                    .displayedName("Pattern")
                    .commit();
            
            STRING_ELEMENT(expected).key("layout.Pattern.format")
                    .description("Set conversion pattern for the layout. See log4cpp documentation.")
                    .displayedName("Format")
                    .assignmentOptional() .defaultValue("%d %c %p %m %n")
                    .commit();
        }


        AppenderConfigurator::AppenderConfigurator(const Hash& input) {
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
            if (input.has("layout.Basic")) {
                m_layout = new krb_log4cpp::BasicLayout();
            } else if (input.has("layout.Simple")) {
                m_layout = new krb_log4cpp::SimpleLayout();
            } else if (input.has("layout.Pattern")) {
                PatternLayout* layout = new krb_log4cpp::PatternLayout();
                layout->setConversionPattern(input.get<string>("layout.Pattern.format"));
                m_layout = layout;
            }
        }


        Appender* AppenderConfigurator::getConfigured() {
            Appender* appender = create();
            assert(appender != 0);
            appender->setLayout(m_layout);
            if (m_threshold != krb_log4cpp::Priority::NOTSET) {
                appender->setThreshold(m_threshold);
            }
            return appender;
        }


    }
}
