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
#include "Logger.hh"
#include "AppenderConfigurator.hh"
#include "CategoryConfigurator.hh"
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"

using namespace std;
using namespace exfel::util;
using namespace log4cpp;


namespace exfel {
    namespace log {

        // Static initialization of logMutex
        boost::mutex Logger::m_logMutex;

        void Logger::expectedParameters(Schema& expected) {



            LIST_ELEMENT<AppenderConfigurator > (expected)
                    .key("appenders")
                    .displayedName("Define Appenders")
                    .description("Configures root appenders")
                    .assignmentOptional().defaultValue("Ostream")
                    .commit();

            STRING_ELEMENT(expected)
                    .description("Default Priority")
                    .key("priority")
                    .displayedName("Priority")
                    .options("DEBUG INFO WARN ERROR")
                    .assignmentOptional().defaultValue("INFO")
                    .commit();


            // Setup for additional categories, optional

            LIST_ELEMENT<CategoryConfigurator > (expected)
                    .key("categories")
                    .displayedName("Categories")
                    .description("Configures categories")
                    .assignmentOptional().noDefaultValue()
                    .commit();



        }

        void Logger::configure(const Hash& input) {

            configureAppenders(input);
            configurePriority(input);
            configureCategories(input);
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

        EXFEL_REGISTER_ONLY_ME_CC(Logger)
    }
}
