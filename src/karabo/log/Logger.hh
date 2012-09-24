/*
 * $Id: Logger.hh 5398 2012-03-07 16:11:30Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 26, 2010, 12:10 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_LOGCONFIG_LOGGER_HH
#define	EXFEL_LOGCONFIG_LOGGER_HH

#include <vector>
#include <string>
#include <karabo/util/Factory.hh>
#include <log4cpp/Priority.hh>
#include <log4cpp/Category.hh>
#include <boost/shared_ptr.hpp>
#include "logdll.hh"
// I belive the following is not needed - see forward declarations
//#include <karabo/util/Hash.hh>
//#include "AppenderConfigurator.hh"
//#include "CategoryConfigurator.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    //forward declarations    
    namespace util {
        class Hash;
    }

    /**
     * Namespace for package log
     */
    namespace log {

        //forward declarations    
        class AppenderConfigurator;
        class CategoryConfigurator;

        /**
         * The Logger class.
         * Configures log4cpp logging system
         */
        class Logger {
        public:

            EXFEL_CLASSINFO(Logger, "Logger", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            virtual ~Logger() {
            }

            /*
             *  Defines the parameters needed to configure log4cpp.
             *  @see exfel::util::Hash and @see exfel::util::Factory
             */
            static void expectedParameters(exfel::util::Schema& expected);

            /*
             *  This method is called by exfel::util::Factory
             */
            void configure(const exfel::util::Hash& input);

            /*
             * This initializes the logger.
             */
            void initialize();

            
            /*
             * This returns a log category reference, automatically configured using the type instrospection system
             * CAVEAT: This function only works for classes that declare the EXFEL_CLASSINFO macro!
             */
            template <class Class>
            static log4cpp::Category& logger() {
                boost::mutex::scoped_lock lock(m_logMutex);
                return log4cpp::Category::getInstance(Class::classInfo().getLogCategory());
            }
            
            static log4cpp::Category& logger(const std::string& logCategory) {
                boost::mutex::scoped_lock lock(m_logMutex);
                return log4cpp::Category::getInstance(logCategory);
            }

        protected:

        private:
            typedef boost::shared_ptr<CategoryConfigurator> CategoryConfiguratorPointer;
            typedef boost::shared_ptr<AppenderConfigurator> AppenderConfiguratorPointer;

            std::vector<boost::shared_ptr<AppenderConfigurator> > m_rootAppenderConfigs;
            log4cpp::Priority::Value m_rootPriority;
            std::vector<CategoryConfiguratorPointer> m_categories;
            static boost::mutex m_logMutex;

            void configureAppenders(const exfel::util::Hash& input);
            void configurePriority(const exfel::util::Hash& input);
            void configureCategories(const exfel::util::Hash& input);

        };
    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::log::Logger, TEMPLATE_LOG, DECLSPEC_LOG)

#endif	/* EXFEL_LOGCONFIG_LOGGER_HH */
