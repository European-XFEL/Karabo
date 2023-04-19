/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "Logger.hh"

#include <karabo/util/ListElement.hh>
#include <karabo/util/SimpleElement.hh>

#include "CacheAppender.hh"
#include "OstreamAppender.hh"
#include "RollingFileAppender.hh"
#include "karabo/util/NodeElement.hh"
#include "krb_log4cpp/Appender.hh"
#include "krb_log4cpp/Category.hh"


using namespace karabo::util;
using namespace std;

namespace karabo {
    namespace log {

        // Static initialization of logMutex
        boost::shared_mutex Logger::m_logMutex;
        boost::shared_mutex Logger::m_frameworkLogMutex;

        // Static initialization of LogStreamRegistry
        Logger::CategoryMap Logger::m_categories;

        Logger::CategorySet Logger::m_frameworkCategories;

        Hash Logger::m_config;


        void Logger::expectedParameters(Schema& s) {
            // Take care to keep this priority in sync with "Logger.priority" of the Python karabo/bound_api/device.py
            STRING_ELEMENT(s)
                  .key("priority")
                  .displayedName("Priority")
                  .description("The default log priority")
                  .options("DEBUG INFO WARN ERROR FATAL")
                  .assignmentOptional()
                  .defaultValue("INFO")
                  .commit();

            NODE_ELEMENT(s).key("ostream").appendParametersOf<OstreamAppender>().commit();

            NODE_ELEMENT(s).key("file").appendParametersOf<RollingFileAppender>().commit();

            NODE_ELEMENT(s).key("cache").appendParametersOf<CacheAppender>().commit();
        }


        void Logger::configure(const karabo::util::Hash& config) {
            // Clear m_config in case a call to configure happened before
            m_config.clear();
            // Validate given configuration
            Schema schema;
            expectedParameters(schema);
            Validator validator; // Default validation
            std::pair<bool, std::string> ret = validator.validate(schema, config, m_config);
            if (ret.first) { // Validation succeeded
                setPriority(m_config.get<string>("priority"));
            } else {
                throw KARABO_PARAMETER_EXCEPTION("Logger configuration failed. \n" + ret.second);
            }
        }


        void Logger::useCache(const std::string& category, bool inheritAppenders) {
            auto p = Configurator<CacheAppender>::createNode("cache", m_config);
            useAppender(category, inheritAppenders, p->getAppender());
        }


        void Logger::useOstream(const std::string& category, bool inheritAppenders) {
            auto p = Configurator<OstreamAppender>::createNode("ostream", m_config);
            useAppender(category, inheritAppenders, p->getAppender());
        }


        void Logger::useFile(const std::string& category, bool inheritAppenders) {
            auto p = Configurator<RollingFileAppender>::createNode("file", m_config);
            useAppender(category, inheritAppenders, p->getAppender());
        }


        void Logger::useAppender(const std::string& category, bool inheritAppenders, krb_log4cpp::Appender* appender) {
            if (m_config.empty()) configure(Hash());
            krb_log4cpp::Category& c = Logger::getCategory(category);
            c.addAppender(appender);
            c.setAdditivity(inheritAppenders);
        }


        void Logger::setPriority(const std::string& priority, const std::string& category) {
            getCategory(category).setPriority(krb_log4cpp::Priority::getPriorityValue(priority));
        }


        const std::string& Logger::getPriority(const std::string& category) {
            return krb_log4cpp::Priority::getPriorityName(getCategory(category).getPriority());
        }

        std::vector<Hash> Logger::getCachedContent(unsigned int nMessages) {
            return CacheAppender::getCachedContent(nMessages);
        }

        void Logger::reset() {
            krb_log4cpp::Category::getRoot().removeAllAppenders();
            // Also remove all nested appenders
            boost::shared_lock<boost::shared_mutex> lock(m_logMutex);
            for (auto it : m_categories) {
                it.second->removeAllAppenders();
            }
        }


        krb_log4cpp::CategoryStream Logger::logDebug(const std::string& category) {
            return getCategory(category).getStream(krb_log4cpp::Priority::DEBUG);
        }


        krb_log4cpp::CategoryStream Logger::logInfo(const std::string& category) {
            return getCategory(category).getStream(krb_log4cpp::Priority::INFO);
        }


        krb_log4cpp::CategoryStream Logger::logWarn(const std::string& category) {
            return getCategory(category).getStream(krb_log4cpp::Priority::WARN);
        }


        krb_log4cpp::CategoryStream Logger::logError(const std::string& category) {
            return getCategory(category).getStream(krb_log4cpp::Priority::ERROR);
        }


        krb_log4cpp::Category& Logger::getCategory(const std::string& category) {
            if (category.empty()) return krb_log4cpp::Category::getRoot();
            boost::upgrade_lock<boost::shared_mutex> lock(m_logMutex);
            CategoryMap::const_iterator it = m_categories.find(category);
            if (it != m_categories.end()) {
                return *(it->second);
            } else {
                boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
                krb_log4cpp::Category* cat = &(krb_log4cpp::Category::getInstance(category));
                m_categories[category] = cat;
                return *cat;
            }
        }


        krb_log4cpp::Category& Logger::getCategory_(const std::string& category) {
            boost::upgrade_lock<boost::shared_mutex> lock(m_frameworkLogMutex);
            if (m_frameworkCategories.find(category) == m_frameworkCategories.end()) {
                boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
                m_frameworkCategories.insert(category);
            }
            return getCategory(category);
        }


        bool Logger::isInCategoryNameSet(const std::string& category) {
            boost::shared_lock<boost::shared_mutex> lock(m_frameworkLogMutex);
            return (m_frameworkCategories.find(category) != m_frameworkCategories.end());
        }
    } // namespace log
} // namespace karabo
