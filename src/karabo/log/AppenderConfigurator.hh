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



#ifndef KARABO_LOGCONFIG_APPENDERCONFIGURATOR_HH
#define	KARABO_LOGCONFIG_APPENDERCONFIGURATOR_HH

#include <string>
#include <karabo/util/Configurator.hh>
#include <krb_log4cpp/Appender.hh>
#include <krb_log4cpp/Priority.hh>
#include <krb_log4cpp/Layout.hh>

namespace karabo {
    namespace log {

        class AppenderConfigurator {

        public:
            KARABO_CLASSINFO(AppenderConfigurator, "Appender", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

        private:

            std::string m_appenderName;
            krb_log4cpp::Priority::Value m_threshold;
            krb_log4cpp::Layout* m_layout;

        protected:

            const std::string& getName() const {
                return m_appenderName;
            }

            virtual krb_log4cpp::Appender* create() = 0;

        public:

            AppenderConfigurator(const std::string& appenderName, krb_log4cpp::Priority::Value threshold, krb_log4cpp::Layout* layout) :
            m_appenderName(appenderName), m_threshold(threshold), m_layout(layout) {
            }

            AppenderConfigurator() :
            m_appenderName("noname"), m_threshold(krb_log4cpp::Priority::NOTSET) {
            }

            virtual ~AppenderConfigurator() {
            }

            virtual krb_log4cpp::Appender* getConfigured();

            static void expectedParameters(karabo::util::Schema& expected);
            AppenderConfigurator(const karabo::util::Hash& input);

        private:
            void configureName(const karabo::util::Hash& input);
            void configureThreshold(const karabo::util::Hash& input);
            void configureLayout(const karabo::util::Hash& input);
        };
    }
}

//TODO Check WINDOWS
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::log::AppenderConfigurator, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* KARABO_LOGCONFIG_APPENDERCONFIGURATOR_HH */

