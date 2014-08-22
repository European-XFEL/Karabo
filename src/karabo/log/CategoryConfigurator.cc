/*
 * $Id: CategoryConfigurator.cc 4647 2011-11-04 16:21:02Z heisenb@DESY.DE $
 *
 * File:   CategoryConfigurator.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <krb_log4cpp/Category.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ListElement.hh>
#include "CategoryConfigurator.hh"


using namespace std;
using namespace krb_log4cpp;
using namespace karabo::util;

namespace karabo {
    namespace log {


        KARABO_REGISTER_FOR_CONFIGURATION(CategoryConfigurator)


        CategoryConfigurator::~CategoryConfigurator() {
        }


        void CategoryConfigurator::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("name")
                    .description("Category name")
                    .displayedName("Name")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("priority")
                    .description("Priority")
                    .displayedName("Priority")
                    .options("DEBUG INFO WARN ERROR")
                    .assignmentOptional().noDefaultValue()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("additivity")
                    .displayedName("Additivity")
                    .description("Set additivity for the category")
                    .assignmentOptional().defaultValue(true)
                    .commit();

            LIST_ELEMENT(expected)
                    .key("appenders")
                    .displayedName("Appender")
                    .description("Configures additional appenders for the category")
                    .appendNodesOfConfigurationBase<AppenderConfigurator>()
                    .assignmentOptional().noDefaultValue()
                    .commit();
        }


        CategoryConfigurator::CategoryConfigurator(const Hash& input) {

            configureName(input);
            configurePriority(input);
            configureAdditivity(input);
            configureAppenders(input);
        }


        void CategoryConfigurator::setup() {

            Category& log = Category::getInstance(m_name);
            log.setPriority(m_level);
            log.setAdditivity(m_additivity);
            log.removeAllAppenders();
            for (size_t i = 0; i < m_appenderConfigurators.size(); ++i) {
                log.addAppender(m_appenderConfigurators[i]->getConfigured());
            }
        }


        void CategoryConfigurator::configureName(const Hash& input) {
            m_name = input.get<string > ("name");
        }


        void CategoryConfigurator::configurePriority(const Hash& input) {
            if (input.has("priority")) {
                string level = input.get<string > ("priority");
                m_level = krb_log4cpp::Priority::getPriorityValue(level);
            } else {
                m_level = krb_log4cpp::Priority::NOTSET;
            }
        }


        void CategoryConfigurator::configureAdditivity(const Hash& input) {
            m_additivity = input.get<bool > ("additivity");
        }


        void CategoryConfigurator::configureAppenders(const Hash& input) {
            // appenders in category are optional
            if (input.has("appenders")) {
                m_appenderConfigurators = AppenderConfigurator::createList("appenders", input);
            }
        }



    }
}
