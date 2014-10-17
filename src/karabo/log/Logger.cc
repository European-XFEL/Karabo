/*
 * $Id: Logger.cc 5319 2012-03-01 13:44:24Z heisenb $
 *
 * File:   Logger.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on September 20, 2010, 10:33 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <vector>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ListElement.hh>
#include "Logger.hh"
#include "AppenderConfigurator.hh"
#include "CategoryConfigurator.hh"
#include "krb_log4cpp/Category.hh"
#include "krb_log4cpp/Appender.hh"

using namespace std;
using namespace karabo::util;
using namespace krb_log4cpp;


namespace karabo {
    namespace log {

        KARABO_REGISTER_FOR_CONFIGURATION(Logger)
        
        // Static initialization of logMutex
        boost::mutex Logger::m_logMutex;
        
        // Static initialization of LogStreamRegistry
        Logger::LogStreamRegistry Logger::m_logStreams;


        void Logger::expectedParameters(Schema& expected) {

            LIST_ELEMENT(expected)
                    .key("appenders")
                    .displayedName("Define Appenders")
                    .description("Configures root appenders")
                    .appendNodesOfConfigurationBase<AppenderConfigurator>()
                    .assignmentOptional().defaultValueFromString("Ostream")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("priority")
                    .description("Default Priority")
                    .displayedName("Priority")
                    .options("DEBUG INFO WARN ERROR")
                    .assignmentOptional().defaultValue("INFO")
                    .commit();
            
            // Setup for additional categories, optional
            LIST_ELEMENT(expected)
                    .key("categories")
                    .displayedName("Categories")
                    .description("Configures categories")
                    .appendNodesOfConfigurationBase<CategoryConfigurator>()
                    .assignmentOptional().noDefaultValue()
                    .commit();
        }


        Logger::Logger(const Hash& input) {
            configureAppenders(input);
            configurePriority(input);
            configureCategories(input);
            initialize();
        }


        void Logger::initialize() {

            Category& rootLog = Category::getRoot();

            rootLog.setRootPriority(m_rootPriority);

            
            
            for (size_t i = 0; i < m_rootAppenderConfigs.size(); ++i) {
                Appender* app = m_rootAppenderConfigs[i]->getConfigured();
                rootLog.addAppender(app);
            }

            for (size_t i = 0; i < m_categories.size(); ++i) {
                m_categories[i]->setup();
            }

        }
        
        void Logger::reset() {
            Category& rootLog = Category::getRoot();
            rootLog.removeAllAppenders();
        }


        void Logger::configureAppenders(const Hash& input) {
            m_rootAppenderConfigs = AppenderConfigurator::createList("appenders", input);
        }


        void Logger::configurePriority(const Hash& input) {
            string prio = input.get<string > ("priority");
            m_rootPriority = Priority::getPriorityValue(prio);
        }


        void Logger::configureCategories(const Hash& input) {
            if (input.has("categories")) {
                m_categories = CategoryConfigurator::createList("categories", input);
            }
        }
    }
}
