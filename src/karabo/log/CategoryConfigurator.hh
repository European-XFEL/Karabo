/*
 * $Id: CategoryConfigurator.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * File:   CategoryConfigurator.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_LOGCONFIG_CATEGORYCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_CATEGORYCONFIGURATOR_HH



#include <string>
#include <vector>
#include "krb_log4cpp/Priority.hh"
#include <karabo/util/Configurator.hh>
#include "AppenderConfigurator.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        class CategoryConfigurator {

            std::string m_name;
            krb_log4cpp::Priority::Value m_level;
            bool m_additivity;
            std::vector<AppenderConfigurator::Pointer> m_appenderConfigurators;

        public:
            KARABO_CLASSINFO(CategoryConfigurator, "Category", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS
           
            virtual ~CategoryConfigurator();

            void setup();

            static void expectedParameters(karabo::util::Schema& expected);
            
            CategoryConfigurator(const karabo::util::Hash& input);

        private:

            void configureName(const karabo::util::Hash& input);
            
            void configurePriority(const karabo::util::Hash& input);
            
            void configureAdditivity(const karabo::util::Hash& input);
            
            void configureAppenders(const karabo::util::Hash& input);
        };

    }
}

// TODO WINDOWS
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::log::CategoryConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* KARABO_LOGCONFIG_CATEGORYCONFIGURATOR_HH */
