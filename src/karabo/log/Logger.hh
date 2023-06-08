/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_LOGCONFIG_LOGGER_HH
#define KARABO_LOGCONFIG_LOGGER_HH

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/type_index.hpp>
#include <krb_log4cpp/Category.hh>
#include <krb_log4cpp/Priority.hh>
#include <unordered_map>
#include <unordered_set>

#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"

namespace karabo {

    namespace log {

        /**
         * The Logger class.
         * Configures log4cpp logging system
         */
        class Logger {
           public:
            KARABO_CLASSINFO(Logger, "Logger", "")

            virtual ~Logger() = default;

            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * Static method allowing to configure the three appenders and the default priority level
             * @param config A hash which must follow the Schema described in the expectedParameters method
             */
            static void configure(const karabo::util::Hash& config);

            /**
             * Enables the cache appender on the specified category.
             *
             * By default any appenders defined on parent categories will be inherited. A boolean flag
             * allows to disable this behavior
             * @param category The category on which the appender should work (empty string reflects root category)
             * @param inheritAppenders If true will inherit appenders defined in parent categories
             */
            static void useCache(const std::string& category = "", bool inheritAppenders = true);

            /**
             * Enables the ostream appender on the specified category.
             *
             * By default any appenders defined on parent categories will be inherited. A boolean flag
             * allows to disable this behavior
             * @param category The category on which the appender should work (empty string reflects root category)
             * @param inheritAppenders If true will inherit appenders defined in parent categories
             */
            static void useOstream(const std::string& category = "", bool inheritAppenders = true);

            /**
             * Enables the rolling file appender on the specified category.
             *
             * By default any appenders defined on parent categories will be inherited. A boolean flag
             * allows to disable this behavior
             * @param category The category on which the appender should work (empty string reflects root category)
             * @param inheritAppenders If true will inherit appenders defined in parent categories
             */
            static void useFile(const std::string& category = "", bool inheritAppenders = true);

            /**
             * Resets all appenders from all categories. Nothing will be logged after a call to this function.
             *
             * Use this function to re-configure logger behavior at runtime.
             */
            static void reset();

            /**
             * Adds a debug message on the defined category
             * @param category The category for this message (empty string reflects root category)
             * @return A stream object that can be used with the \<\< operator
             */
            static krb_log4cpp::CategoryStream logDebug(const std::string& category = "");

            /**
             * Adds a info message on the defined category
             * @param category The category for this message (empty string reflects root category)
             * @return A stream object that can be used with the \<\< operator
             */
            static krb_log4cpp::CategoryStream logInfo(const std::string& category = "");

            /**
             * Adds a warn message on the defined category
             * @param category The category for this message (empty string reflects root category)
             * @return A stream object that can be used with the \<\< operator
             */
            static krb_log4cpp::CategoryStream logWarn(const std::string& category = "");

            /**
             * Adds a error message on the defined category
             * @param category The category for this message (empty string reflects root category)
             * @return A stream object that can be used with the \<\< operator
             */
            static krb_log4cpp::CategoryStream logError(const std::string& category = "");

            /**
             * Allows to set the priority filter on specified category
             * @param priority DEBUG,INFO,WARN of ERROR
             * @param category The category to apply the filter to (empty string reflects root category)
             */
            static void setPriority(const std::string& priority, const std::string& category = "");

            /**
             * Retrieve the currently enabled priority level for the given category
             * @param category The category (empty string reflects root category)
             */
            static const std::string& getPriority(const std::string& category = "");

            /**
             * This returns a log category reference, automatically configured using the type introspection system
             * CAVEAT: This function only works for classes that declare the KARABO_CLASSINFO macro!
             * @return a log4cpp category object
             */
            template <typename T>
            static krb_log4cpp::Category& getCategory();

            /**
             * Retrieves a log4cpp category object for specified name.
             * Categories are created on demand and cached from then on
             * @param category Name for the category (empty string reflects root category)
             * @return a log4cpp category object
             */
            static krb_log4cpp::Category& getCategory(const std::string& category = std::string());

            /**
             * Register category name in category name set and
             * call getCategory(category)
             * @param category name
             * @return log4cpp Category object
             */
            static krb_log4cpp::Category& getCategory_(const std::string& category = std::string());

            /**
             * Check if category name is found in the category name set
             * @param category name
             * @return true if found else false
             */
            static bool isInCategoryNameSet(const std::string& category);

            /**
             * gets the last messages from the CacheAppender
             * @param nMessages
             * @return a std::vector<std::string>
             */
            static std::vector<karabo::util::Hash> getCachedContent(unsigned int nMessages);

           private:
            static void useAppender(const std::string& category, bool inheritAppenders, krb_log4cpp::Appender*);

            Logger() = default;

            typedef std::unordered_map<std::string, krb_log4cpp::Category*> CategoryMap;
            static CategoryMap m_categories;

            // Declare FRAMEWORK category name set
            typedef std::unordered_set<std::string> CategorySet;
            static CategorySet m_frameworkCategories;
            static boost::shared_mutex m_logMutex;
            static boost::shared_mutex m_frameworkLogMutex;

            static karabo::util::Hash m_config;
        };

        template <typename T>
        krb_log4cpp::Category& Logger::getCategory() {
            return getCategory_(T::classInfo().getLogCategory());
        }
    } // namespace log
} // namespace karabo
#endif

// Convenient logging
#ifdef KARABO_ENABLE_TRACE_LOG
#define KARABO_LOG_FRAMEWORK_TRACE KARABO_LOG_FRAMEWORK_DEBUG
#define KARABO_LOG_FRAMEWORK_TRACE_C(category) KARABO_LOG_FRAMEWORK_DEBUG_C(category)
#define KARABO_LOG_FRAMEWORK_TRACE_CF KARABO_LOG_FRAMEWORK_DEBUG_C(Self::classInfo().getLogCategory() + "." + __func__)
#else
#define KARABO_LOG_FRAMEWORK_TRACE \
    if (1)                         \
        ;                          \
    else std::cerr
#define KARABO_LOG_FRAMEWORK_TRACE_C(category) \
    if (1)                                     \
        ;                                      \
    else std::cerr
#define KARABO_LOG_FRAMEWORK_TRACE_CF \
    if (1)                            \
        ;                             \
    else std::cerr
#endif

#define KARABO_LOG_FRAMEWORK_DEBUG karabo::log::Logger::getCategory<Self>() << krb_log4cpp::Priority::DEBUG
#define KARABO_LOG_FRAMEWORK_INFO karabo::log::Logger::getCategory<Self>() << krb_log4cpp::Priority::INFO
#define KARABO_LOG_FRAMEWORK_WARN karabo::log::Logger::getCategory<Self>() << krb_log4cpp::Priority::WARN
#define KARABO_LOG_FRAMEWORK_ERROR karabo::log::Logger::getCategory<Self>() << krb_log4cpp::Priority::ERROR

#define KARABO_LOG_FRAMEWORK_DEBUG_C(category) \
    karabo::log::Logger::getCategory_(category) << krb_log4cpp::Priority::DEBUG
#define KARABO_LOG_FRAMEWORK_INFO_C(category) karabo::log::Logger::getCategory_(category) << krb_log4cpp::Priority::INFO
#define KARABO_LOG_FRAMEWORK_WARN_C(category) karabo::log::Logger::getCategory_(category) << krb_log4cpp::Priority::WARN
#define KARABO_LOG_FRAMEWORK_ERROR_C(category) \
    karabo::log::Logger::getCategory_(category) << krb_log4cpp::Priority::ERROR
