/*
 * $Id: Logger.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 * Contributions: <burkhard.heisen@xfel.eu>
 *
 * Created on August 26, 2010, 12:10 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_LOGCONFIG_LOGGER_HH
#define	KARABO_LOGCONFIG_LOGGER_HH

#include <vector>
#include <string>
#include <karabo/util/Configurator.hh>
#include <krb_log4cpp/Priority.hh>
#include <krb_log4cpp/Category.hh>
#include <boost/shared_ptr.hpp>

#include "AppenderConfigurator.hh"
#include "CategoryConfigurator.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package log
     */
    namespace log {

        /**
         * The Logger class.
         * Configures log4cpp logging system
         */
        class Logger {

            typedef std::map<std::string, krb_log4cpp::Category*> LogStreamRegistry;

            std::vector<AppenderConfigurator::Pointer> m_rootAppenderConfigs;

            krb_log4cpp::Priority::Value m_rootPriority;

            std::vector<CategoryConfigurator::Pointer> m_categories;

            static boost::mutex m_logMutex;

            static LogStreamRegistry m_logStreams;

        public:

            KARABO_CLASSINFO(Logger, "Logger", "1.0")

            virtual ~Logger() {
            }

            /*
             *  Defines the parameters needed to configure log4cpp.
             */
            static void expectedParameters(karabo::util::Schema& expected);

            /*
             *  This constructor is called by karabo::util::Configurator
             */
            Logger(const karabo::util::Hash& input);

            static void configure(const karabo::util::Hash& configuration = karabo::util::Hash()) {
                karabo::util::Configurator<Logger>::create(classInfo().getClassId(), configuration);
            }
            
            static void reset();
            
            /*
             * This returns a log category reference, automatically configured using the type introspection system
             * CAVEAT: This function only works for classes that declare the KARABO_CLASSINFO macro!
             */
            template <class Class>
            static krb_log4cpp::Category& getLogger() {
                return getLogger(Class::classInfo().getLogCategory());
            }

            static krb_log4cpp::Category& getLogger(const std::string& logCategory) {
                LogStreamRegistry::const_iterator it = m_logStreams.find(logCategory);
                if (it != m_logStreams.end()) {
                    return *(it->second);
                } else {
                    boost::mutex::scoped_lock lock(m_logMutex);
                    krb_log4cpp::Category* tmp = &(krb_log4cpp::Category::getInstance(logCategory));
                    m_logStreams[logCategory] = tmp;
                    return *tmp;
                }
            }

        private:

            /*
             * This initializes the logger.
             */
            void initialize();

            void configureAppenders(const karabo::util::Hash& input);

            void configurePriority(const karabo::util::Hash& input);

            void configureCategories(const karabo::util::Hash& input);

        };

        // Convenient logging
        #ifdef KARABO_ENABLE_TRACE_LOG
        #define KARABO_LOG_FRAMEWORK_TRACE KARABO_LOG_FRAMEWORK_DEBUG
        #define KARABO_LOG_FRAMEWORK_TRACE_C(category) KARABO_LOG_FRAMEWORK_DEBUG_C(category)
        #define KARABO_LOG_FRAMEWORK_TRACE_CF KARABO_LOG_FRAMEWORK_DEBUG_C(Self::classInfo().getLogCategory() + "." + __func__)        
        #else 
        #define KARABO_LOG_FRAMEWORK_TRACE if(1); else std::cerr
        #define KARABO_LOG_FRAMEWORK_TRACE_C(category) if(1); else std::cerr
        #define KARABO_LOG_FRAMEWORK_TRACE_CF if(1); else std::cerr
        #endif
        
        #define KARABO_LOG_FRAMEWORK_DEBUG karabo::log::Logger::getLogger<Self>() << krb_log4cpp::Priority::DEBUG 
        #define KARABO_LOG_FRAMEWORK_INFO  karabo::log::Logger::getLogger<Self>() << krb_log4cpp::Priority::INFO 
        #define KARABO_LOG_FRAMEWORK_WARN  karabo::log::Logger::getLogger<Self>() << krb_log4cpp::Priority::WARN 
        #define KARABO_LOG_FRAMEWORK_ERROR karabo::log::Logger::getLogger<Self>() << krb_log4cpp::Priority::ERROR

        #define KARABO_LOG_FRAMEWORK_DEBUG_C(category) karabo::log::Logger::getLogger(category) << krb_log4cpp::Priority::DEBUG 
        #define KARABO_LOG_FRAMEWORK_INFO_C(category)  karabo::log::Logger::getLogger(category) << krb_log4cpp::Priority::INFO 
        #define KARABO_LOG_FRAMEWORK_WARN_C(category)  karabo::log::Logger::getLogger(category) << krb_log4cpp::Priority::WARN 
        #define KARABO_LOG_FRAMEWORK_ERROR_C(category) karabo::log::Logger::getLogger(category) << krb_log4cpp::Priority::ERROR 

    }
}

// TODO WINDOWS
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::log::Logger, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* KARABO_LOGCONFIG_LOGGER_HH */
