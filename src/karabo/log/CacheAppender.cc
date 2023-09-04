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


#include "CacheAppender.hh"

#include <iostream>
#include <karabo/util/SimpleElement.hh>
#include <krb_log4cpp/Appender.hh>
#include <krb_log4cpp/LoggingEvent.hh>
#include <krb_log4cpp/PatternLayout.hh>
#include <string>


using namespace std;
using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::log::CacheAppender)

namespace karabo {
    namespace log {

        void CacheAppender::expectedParameters(Schema& s) {
            STRING_ELEMENT(s)
                  .key("threshold")
                  .description(
                        "The Appender will not appended log events with a priority lower than the threshold.\
                                  Use Priority::NOTSET to disable threshold checking.")
                  .displayedName("Threshold")
                  .options({"NOTSET", "DEBUG", "INFO", "WARN", "ERROR"})
                  .assignmentOptional()
                  .defaultValue("INFO")
                  .commit();

            UINT32_ELEMENT(s)
                  .key("maxNumMessages")
                  .displayedName("Max. Num. Messages")
                  .description("Maximum number of messages in the cache")
                  .assignmentOptional()
                  .defaultValue(100u)
                  .maxInc(1000u)
                  .commit();
        }

        CacheAppender::CacheAppender(const Hash& config) {
            LogCache::getInstance(config.get<unsigned int>("maxNumMessages"));
            // This raw pointer will be handed to a log4cpp category object (functionality of Logger class)
            // Log4cpp will take ownership and deal with memory management (proper delete)
            m_appender = new Log4CppCacheApp();
            m_appender->setThreshold(krb_log4cpp::Priority::getPriorityValue(config.get<string>("threshold")));
        }

        krb_log4cpp::Appender* CacheAppender::getAppender() {
            return m_appender;
        }

        std::vector<Hash> CacheAppender::getCachedContent(unsigned int numMessages) {
            auto p = LogCache::getInstance();
            if (!p) {
                return {};
            }
            return p->getCachedContent(numMessages);
        }

        void CacheAppender::reset() {
            auto p = LogCache::getInstance();
            if (!p) return;
            p->reset();
        }


        Log4CppCacheApp::Log4CppCacheApp() : LayoutAppender("CacheAppender") {
            // Time format should be ISO8601, but the log4cpp version misses a `T` adding here.
            m_timeLayout.setConversionPattern("%d{%Y-%m-%dT%H:%M:%S.%l}");
            m_priorityLayout.setConversionPattern("%p"); // DEBUG, INFO, WARN or ERROR
            m_categoryLayout.setConversionPattern("%c"); // deviceId
            m_messageLayout.setConversionPattern("%m");  // message text
        }

        void Log4CppCacheApp::_append(const krb_log4cpp::LoggingEvent& event) {
            // clang-format off
            Hash entry(
                "timestamp", m_timeLayout.format(event),
                "type", m_priorityLayout.format(event),
                "category", m_categoryLayout.format(event),
                "message", m_messageLayout.format(event));
            // clang-format on
            LogCache::getInstance()->append(entry);
        }


        bool Log4CppCacheApp::reopen() {
            return true;
        }


        LogCache::Pointer LogCache::m_instance;
        std::once_flag LogCache::m_instanceFlag;


        LogCache::Pointer LogCache::getInstance(unsigned int maxMessages) {
            if (!m_instance) {
                call_once(m_instanceFlag, [maxMessages]() { LogCache::init(maxMessages); });
            }
            // the LogCache is a static shared item:
            // If one requestor wants a larger cache, the cache will fit to the largest requested size
            if (maxMessages > 0) {
                m_instance->setSize(maxMessages);
            }
            return m_instance;
        }


        void LogCache::init(unsigned int maxMessages) {
            m_instance.reset(new LogCache(maxMessages));
        }


        LogCache::LogCache(unsigned int maxMessages) : m_maxMessages(maxMessages) {}


        void LogCache::setSize(unsigned int maxMessages) {
            if (m_maxMessages < maxMessages) {
                m_maxMessages = maxMessages;
            }
        }


        std::vector<Hash> LogCache::getCachedContent(unsigned int numMessages) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (numMessages < m_logCache.size()) {
                return {m_logCache.end() - numMessages, m_logCache.end()};
            }
            return m_logCache;
        }


        void LogCache::reset() {
            m_logCache.clear();
        }


        void LogCache::append(const Hash& entry) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_logCache.size() == m_maxMessages) {
                m_logCache.erase(m_logCache.begin());
            }
            m_logCache.push_back(entry);
        }
    } // namespace log
} // namespace karabo
