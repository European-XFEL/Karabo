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
#include <karabo/util/util.hh>
#include <log4cpp/Priority.hh>
#include <log4cpp/Category.hh>
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

            typedef std::map<std::string, log4cpp::Category*> LogStreamRegistry;

            std::vector<AppenderConfigurator::Pointer> m_rootAppenderConfigs;

            log4cpp::Priority::Value m_rootPriority;

            std::vector<CategoryConfigurator::Pointer> m_categories;

            static boost::mutex m_logMutex;

            static LogStreamRegistry m_logStreams;

        public:

            KARABO_CLASSINFO(Logger, "Logger", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

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

            /*
             * This initializes the logger.
             */
            void initialize();

            /*
             * This returns a log category reference, automatically configured using the type introspection system
             * CAVEAT: This function only works for classes that declare the KARABO_CLASSINFO macro!
             */
            template <class Class>
            static log4cpp::Category& getLogger() {
                return getLogger(Class::classInfo().getLogCategory());
            }

            static log4cpp::Category& getLogger(const std::string& logCategory) {
                LogStreamRegistry::const_iterator it = m_logStreams.find(logCategory);
                if (it != m_logStreams.end()) {
                    return *(it->second);
                } else {
                    boost::mutex::scoped_lock lock(m_logMutex);
                    log4cpp::Category* tmp = &(log4cpp::Category::getInstance(logCategory));
                    m_logStreams[logCategory] = tmp;
                    return *tmp;
                }
            }

        private:

            void configureAppenders(const karabo::util::Hash& input);

            void configurePriority(const karabo::util::Hash& input);

            void configureCategories(const karabo::util::Hash& input);

        };

        // Convenient logging
        #ifdef KARABO_ENABLE_TRACE_LOG
        #define KARABO_LOG_TRACE if(0); else std::cerr
        #else 
        #define KARABO_LOG_TRACE if(1); else std::cerr
        #endif
        
        #define _KARABO_LOG_DEBUG_0 Logger::getLogger<Self>() << log4cpp::Priority::DEBUG 
        #define _KARABO_LOG_INFO_0  Logger::getLogger<Self>() << log4cpp::Priority::INFO 
        #define _KARABO_LOG_WARN_0  Logger::getLogger<Self>() << log4cpp::Priority::WARN 
        #define _KARABO_LOG_ERROR_0 Logger::getLogger<Self>() << log4cpp::Priority::ERROR
        
        #define _KARABO_LOG_DEBUG_1(category) Logger::getLogger(category) << log4cpp::Priority::DEBUG 
        #define _KARABO_LOG_INFO_1(category)  Logger::getLogger(category) << log4cpp::Priority::INFO 
        #define _KARABO_LOG_WARN_1(category)  Logger::getLogger(category) << log4cpp::Priority::WARN 
        #define _KARABO_LOG_ERROR_1(category) Logger::getLogger(category) << log4cpp::Priority::ERROR 

        #define _KARABO_LOG_DEBUG_N(x0,x1,FUNC, ...) FUNC
        
        #define KARABO_LOG_DEBUG(...) \
                    _KARABO_LOG_DEBUG_N(,##__VA_ARGS__, \
                    _KARABO_LOG_DEBUG_1(__VA_ARGS__), \
                    _KARABO_LOG_DEBUG_0(__VA_ARGS__) \
                    )
        
        #define KARABO_LOG_INFO(...) \
                    _KARABO_LOG_INFO_N(,##__VA_ARGS__, \
                    _KARABO_LOG_INFO_1(__VA_ARGS__), \
                    _KARABO_LOG_INFO_0(__VA_ARGS__) \
                    )
        
        #define KARABO_LOG_WARN(...) \
                    _KARABO_LOG_WARN_N(,##__VA_ARGS__, \
                    _KARABO_LOG_WARN_1(__VA_ARGS__), \
                    _KARABO_LOG_WARN_0(__VA_ARGS__) \
                    )
        
        #define KARABO_LOG_ERROR(...) \
                    _KARABO_LOG_ERROR_N(,##__VA_ARGS__, \
                    _KARABO_LOG_ERROR_1(__VA_ARGS__), \
                    _KARABO_LOG_ERROR_0(__VA_ARGS__) \
                    )
        
    }
}

// TODO WINDOWS
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::log::Logger, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* KARABO_LOGCONFIG_LOGGER_HH */
